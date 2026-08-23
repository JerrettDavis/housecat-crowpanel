Import("env")

from os.path import exists, join
import re
from subprocess import check_call

revision = "216da3300f0493b0860247d43f7af5ce29df63a5"
checkout = join(env.subst("$PROJECT_BUILD_DIR"), "microlink-src")
if not exists(join(checkout, ".git")):
    check_call(["git", "clone", "--filter=blob:none", "https://github.com/CamM2325/microlink.git", checkout])
    check_call(["git", "-C", checkout, "checkout", "--detach", revision])

# Restore the pinned upstream sources before applying the deterministic
# compatibility transforms below. PlatformIO can execute pre-scripts more than
# once in one command (build followed by upload), so transforms must never
# accumulate in the shared checkout.
check_call([
    "git", "-C", checkout, "checkout", "--",
    "components/microlink/src/ml_noise.c",
    "components/microlink/src/ml_wg_mgr.c",
    "components/microlink/include/microlink_internal.h",
    "components/microlink/components/wireguard_lwip/src/wireguardif.c",
    "components/microlink/components/wireguard_lwip/src/wireguard-platform-esp32.c",
])

microlink = join(checkout, "components", "microlink")
wireguard = join(microlink, "components", "wireguard_lwip")

# Keep the tunnel's own coordination traffic on physical WiFi after the VPN
# becomes the default route; otherwise those sockets recursively enter WG.
internal_path = join(microlink, "include", "microlink_internal.h")
with open(internal_path, "r", encoding="utf-8") as source_file:
    internal_source = source_file.read()
internal_source = internal_source.replace(
    "#define ml_socket       socket",
    '''static inline int ml_wifi_socket(int domain, int type, int protocol) {
    int fd = socket(domain, type, protocol);
    if (fd < 0) return fd;
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    struct ifreq iface = {0};
    if (!sta || !if_indextoname(esp_netif_get_netif_impl_index(sta), iface.ifr_name) ||
        setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, &iface, sizeof(iface)) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}
#define ml_socket       ml_wifi_socket''')
if "ml_wifi_socket" not in internal_source:
    raise RuntimeError("Pinned MicroLink socket wrapper section did not match")
internal_source = internal_source.replace(
    '#include "esp_heap_caps.h"',
    '#include "esp_heap_caps.h"\n#include "esp_netif.h"\n#include "lwip/netif.h"\n#include <net/if.h>')
with open(internal_path, "w", encoding="utf-8", newline="\n") as source_file:
    source_file.write(internal_source)

# Arduino's precompiled ESP-IDF libraries omit MBEDTLS_CHACHAPOLY_C. MicroLink
# enables it in its native ESP-IDF sdkconfig, but PlatformIO's Arduino build
# cannot rebuild that archive. Use the identical ChaCha20-Poly1305 reference
# implementation already bundled with wireguard-lwip. The checkout is pinned,
# and exact marker matching makes an upstream layout change fail loudly.
noise_path = join(microlink, "src", "ml_noise.c")
with open(noise_path, "r", encoding="utf-8") as source_file:
    noise_source = source_file.read()
replacement = r'''static int noise_chachapoly_encrypt(const uint8_t *key, uint64_t nonce,
                                      const uint8_t *ad, size_t ad_len,
                                      const uint8_t *plaintext, size_t pt_len,
                                      uint8_t *ciphertext) {
    chacha20poly1305_encrypt(ciphertext, plaintext, pt_len, ad, ad_len,
                             __builtin_bswap64(nonce), key);
    return 0;
}

static int noise_chachapoly_decrypt(const uint8_t *key, uint64_t nonce,
                                      const uint8_t *ad, size_t ad_len,
                                      const uint8_t *ciphertext, size_t ct_len,
                                      uint8_t *plaintext) {
    if (ct_len < 16) return -1;
    return chacha20poly1305_decrypt(plaintext, ciphertext, ct_len, ad, ad_len,
                                    __builtin_bswap64(nonce), key) ? 0 : -1;
}

/* ============================================================================
 * Noise IK Handshake'''
if "static int noise_chachapoly_encrypt" not in noise_source:
    noise_source = noise_source.replace('#include "mbedtls/chacha20.h"\n#include "mbedtls/chachapoly.h"',
                                        '#include "chacha20poly1305.h"')
    noise_source, replacement_count = re.subn(
        r'static int chacha20poly1305_encrypt\(.*?/\* ============================================================================\n \* Noise IK Handshake',
        replacement,
        noise_source,
        count=1,
        flags=re.DOTALL)
    if replacement_count != 1:
        raise RuntimeError("Pinned MicroLink Noise AEAD section did not match")
    noise_source = noise_source.replace("chacha20poly1305_encrypt(k,", "noise_chachapoly_encrypt(k,")
    noise_source = noise_source.replace("chacha20poly1305_decrypt(k,", "noise_chachapoly_decrypt(k,")
    noise_source = noise_source.replace("chacha20poly1305_encrypt(key,", "noise_chachapoly_encrypt(key,")
    noise_source = noise_source.replace("chacha20poly1305_decrypt(key,", "noise_chachapoly_decrypt(key,")
with open(noise_path, "w", encoding="utf-8", newline="\n") as source_file:
    source_file.write(noise_source)

# MicroLink's netif management predates ESP-IDF 5.5's mandatory lwIP core-lock
# assertions. Its manager runs in a FreeRTOS worker, so every mutation of the
# global netif list/state and raw UDP PCB must hold the TCP/IP core lock.
wg_mgr_path = join(microlink, "src", "ml_wg_mgr.c")
with open(wg_mgr_path, "r", encoding="utf-8") as source_file:
    wg_mgr_source = source_file.read()
if "HOUSECAT_IDF55_CORE_LOCK" not in wg_mgr_source:
    wg_mgr_source = wg_mgr_source.replace(
        '#include "lwip/netif.h"',
        '#include "lwip/netif.h"\n#include "lwip/tcpip.h"\n#include "esp_netif.h"\n#include "esp_netif_net_stack.h"\n#define HOUSECAT_IDF55_CORE_LOCK 1')
    wg_mgr_source = wg_mgr_source.replace(
        '    /* Add to lwIP netif list (bypass netif_add which wants init callback) */',
        '    LOCK_TCPIP_CORE();\n\n    /* Add to lwIP netif list (bypass netif_add which wants init callback) */')
    wg_mgr_source = wg_mgr_source.replace(
        '    /* Register output callbacks for magicsock mode */',
        '    UNLOCK_TCPIP_CORE();\n\n    /* Register output callbacks for magicsock mode */')
    old_shutdown = '''        wireguardif_shutdown(netif);
        netif_set_link_down(netif);
        netif_set_down(netif);
        vTaskDelay(pdMS_TO_TICKS(100));
        netif_remove(netif);'''
    new_shutdown = '''        LOCK_TCPIP_CORE();
        wireguardif_shutdown(netif);
        netif_set_link_down(netif);
        netif_set_down(netif);
        UNLOCK_TCPIP_CORE();
        vTaskDelay(pdMS_TO_TICKS(100));
        LOCK_TCPIP_CORE();
        netif_remove(netif);
        UNLOCK_TCPIP_CORE();'''
    if old_shutdown not in wg_mgr_source:
        raise RuntimeError("Pinned MicroLink shutdown section did not match")
    wg_mgr_source = wg_mgr_source.replace(old_shutdown, new_shutdown)
# Put WG after physical netifs, bind its encapsulation PCB to WiFi, and make it
# default only after the configured exit peer has been installed successfully.
wg_mgr_source = wg_mgr_source.replace(
    '                                dev->peers[p->wg_peer_index].active = false;',
    '                                if (!(ml->config.priority_peer_ip != 0 &&\n'
    '                                      p->vpn_ip == ml->config.priority_peer_ip))\n'
    '                                    dev->peers[p->wg_peer_index].active = false;')
wg_mgr_source = wg_mgr_source.replace(
    '    IP4_ADDR(&netif->netmask.u_addr.ip4, 255, 192, 0, 0);     /* /10 */',
    '    IP4_ADDR(&netif->netmask.u_addr.ip4, 0, 0, 0, 0);         /* public fallback */')
wg_mgr_source = wg_mgr_source.replace(
    '    netif->next = netif_list;\n    netif_list = netif;',
    '    netif->next = NULL;\n    if (!netif_list) netif_list = netif;\n'
    '    else {\n        struct netif *tail = netif_list;\n'
    '        while (tail->next) tail = tail->next;\n        tail->next = netif;\n    }')
wg_mgr_source = wg_mgr_source.replace(
    '            s_wg_output_pcb->tos = 0xB8;',
    '            s_wg_output_pcb->tos = 0xB8;\n'
    '            esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");\n'
    '            if (sta) udp_bind_netif(s_wg_output_pcb,\n'
    '                (struct netif *)esp_netif_get_netif_impl(sta));')
wg_mgr_source = wg_mgr_source.replace(
    '        IP4_ADDR(&wg_peer.allowed_ip.u_addr.ip4, ip_a, ip_b, ip_c, ip_d);\n'
    '        IP4_ADDR(&wg_peer.allowed_mask.u_addr.ip4, 255, 255, 255, 255);',
    '        if (ml->config.priority_peer_ip != 0 && p->vpn_ip == ml->config.priority_peer_ip) {\n'
    '            IP4_ADDR(&wg_peer.allowed_ip.u_addr.ip4, 0, 0, 0, 0);\n'
    '            IP4_ADDR(&wg_peer.allowed_mask.u_addr.ip4, 0, 0, 0, 0);\n'
    '        } else {\n'
    '            IP4_ADDR(&wg_peer.allowed_ip.u_addr.ip4, ip_a, ip_b, ip_c, ip_d);\n'
    '            IP4_ADDR(&wg_peer.allowed_mask.u_addr.ip4, 255, 255, 255, 255);\n'
    '        }')
wg_mgr_source = wg_mgr_source.replace(
    '            p->wg_peer_index = wg_peer_idx;\n',
    '            p->wg_peer_index = wg_peer_idx;\n\n'
    '            if (ml->config.priority_peer_ip != 0 && p->vpn_ip == ml->config.priority_peer_ip) {\n'
    '                LOCK_TCPIP_CORE();\n                netif_set_default(netif);\n'
    '                UNLOCK_TCPIP_CORE();\n'
    '                printf("[tailscale-route] exit node enabled through %s\\n", p->hostname);\n'
    '                wireguardif_connect(netif, wg_peer_idx);\n'
    '            }\n')
wg_mgr_source = wg_mgr_source.replace(
    '    microlink_ip_to_str(update->vpn_ip, ip_str);\n',
    '    microlink_ip_to_str(update->vpn_ip, ip_str);\n'
    '    printf("[tailscale-route] peer %s=%s priority=%d\\n",\n'
    '             p->hostname, ip_str, p->vpn_ip == ml->config.priority_peer_ip);\n')
wg_mgr_source = wg_mgr_source.replace(
    '    err_t err = udp_sendto(s_wg_output_pcb, p, &dst, dest_port);\n',
    '    const bool already_locked = sys_thread_tcpip(LWIP_CORE_LOCK_QUERY_HOLDER);\n'
    '    if (!already_locked) LOCK_TCPIP_CORE();\n'
    '    err_t err = udp_sendto(s_wg_output_pcb, p, &dst, dest_port);\n'
    '    if (!already_locked) UNLOCK_TCPIP_CORE();\n')
with open(wg_mgr_path, "w", encoding="utf-8", newline="\n") as source_file:
    source_file.write(wg_mgr_source)

wireguardif_path = join(wireguard, "src", "wireguardif.c")
with open(wireguardif_path, "r", encoding="utf-8") as source_file:
    wireguardif_source = source_file.read()
wireguardif_source = wireguardif_source.replace(
    '#include "lwip/udp.h"',
    '#include "lwip/udp.h"\n#include "housecat/network_policy.h"')
if "HOUSECAT_IDF55_CORE_LOCK" not in wireguardif_source:
    wireguardif_source = wireguardif_source.replace(
        '#include "lwip/udp.h"',
        '#include "lwip/udp.h"\n#include "lwip/tcpip.h"\n#define HOUSECAT_IDF55_CORE_LOCK 1')
    wireguardif_source = wireguardif_source.replace(
        'netif_set_link_up(device->netif);',
        'LOCK_TCPIP_CORE();\n\t\tnetif_set_link_up(device->netif);\n\t\tUNLOCK_TCPIP_CORE();')
    wireguardif_source = wireguardif_source.replace(
        'netif_set_link_down(device->netif);',
        'LOCK_TCPIP_CORE();\n\t\tnetif_set_link_down(device->netif);\n\t\tUNLOCK_TCPIP_CORE();')
wireguardif_source = wireguardif_source.replace(
    '\t\t\t\t\t\tip_input(pbuf, device->netif);',
    '\t\t\t\t\t\tdevice->netif->input(pbuf, device->netif);')
# Exit-node routing needs longest-prefix matching so normal /32 tailnet peers
# beat the gateway's /0. It also rejects all RFC1918 destinations on WG: local
# IoT services stay on WiFi, while the main VLAN can never be reached via exit.
old_lookup = re.search(
    r'static struct wireguard_peer \*peer_lookup_by_allowed_ip\(.*?\n\}',
    wireguardif_source, re.DOTALL)
if not old_lookup:
    raise RuntimeError("Pinned WireGuard peer lookup did not match")
new_lookup = '''static int housecat_mask_bits(const ip_addr_t *mask) {
    return __builtin_popcount(lwip_ntohl(ip4_addr_get_u32(ip_2_ip4(mask))));
}

static struct wireguard_peer *peer_lookup_by_allowed_ip(struct wireguard_device *device, const ip_addr_t *ipaddr) {
    struct wireguard_peer *best = NULL;
    int best_bits = -1;
    bool best_ready = false;
    for (int x = 0; x < WIREGUARD_MAX_PEERS; x++) {
        struct wireguard_peer *peer = &device->peers[x];
        if (!peer->valid) continue;
        for (int y = 0; y < WIREGUARD_MAX_SRC_IPS; y++) {
            if (!peer->allowed_source_ips[y].valid ||
                !IP_ADDR_NETCMP_COMPAT(ipaddr, &peer->allowed_source_ips[y].ip,
                                       &peer->allowed_source_ips[y].mask)) continue;
            int bits = housecat_mask_bits(&peer->allowed_source_ips[y].mask);
            bool ready = peer->curr_keypair.valid || peer->prev_keypair.valid;
            if (bits > best_bits || (bits == best_bits && ready && !best_ready)) {
                best = peer;
                best_bits = bits;
                best_ready = ready;
            }
        }
    }
    return best;
}'''
wireguardif_source = (wireguardif_source[:old_lookup.start()] + new_lookup +
                      wireguardif_source[old_lookup.end():])
wireguardif_source = wireguardif_source.replace(
    '\tstruct wireguard_device *device = (struct wireguard_device *)netif->state;\n\t// Send to peer that matches dest IP',
    '''\tstruct wireguard_device *device = (struct wireguard_device *)netif->state;
\tuint32_t destination = lwip_ntohl(ip4_addr_get_u32(ipaddr));
\tuint8_t inner_header[24] = {0};
\tu16_t copied = pbuf_copy_partial(q, inner_header, sizeof(inner_header), 0);
\tu8_t ihl = copied > 0 ? (inner_header[0] & 0x0F) * 4 : 0;
\tu16_t destination_port = (copied >= ihl + 4 && ihl >= 20)
\t    ? ((u16_t)inner_header[ihl + 2] << 8) | inner_header[ihl + 3] : 0;
\tconst bool home_assistant_mqtt = HOUSECAT_MQTT_REMOTE_IPV4 != 0UL &&
\t    destination == HOUSECAT_MQTT_REMOTE_IPV4 && inner_header[9] == 6 &&
\t    destination_port == HOUSECAT_MQTT_PORT;
\tif (!home_assistant_mqtt && ((destination >> 24) == 10 ||
\t    (destination >= 0xAC100000UL && destination <= 0xAC1FFFFFUL) ||
\t    (destination >> 16) == 0xC0A8 ||
\t    (destination >> 16) == 0xA9FE)) {
\t\tWG_DEBUG("[WG_OUTPUT] blocked private destination\\n");
\t\treturn ERR_RTE;
\t}
\t// Send to peer that matches dest IP''')
with open(wireguardif_path, "w", encoding="utf-8", newline="\n") as source_file:
    source_file.write(wireguardif_source)

# WireGuard responders reject non-increasing TAI64N values. Upstream uses
# esp_timer uptime, which goes backwards on every reboot. The manager waits for
# SNTP before MicroLink starts, so use epoch time plus a sub-second component.
platform_path = join(wireguard, "src", "wireguard-platform-esp32.c")
with open(platform_path, "r", encoding="utf-8") as source_file:
    platform_source = source_file.read()
platform_source = platform_source.replace(
    '#include <string.h>', '#include <string.h>\n#include <time.h>')
platform_source = platform_source.replace(
    '    uint64_t now_us = esp_timer_get_time();\n'
    '    uint64_t seconds = now_us / 1000000ULL;\n'
    '    uint32_t nanoseconds = (now_us % 1000000ULL) * 1000;',
    '    uint64_t now_us = esp_timer_get_time();\n'
    '    uint64_t seconds = (uint64_t)time(NULL);\n'
    '    uint32_t nanoseconds = (now_us % 1000000ULL) * 1000;')
platform_source = platform_source.replace(
    'printf("[TAI64N] uptime=%llu s, nano=%lu\\n",',
    'printf("[TAI64N] epoch=%llu s, nano=%lu\\n",')
with open(platform_path, "w", encoding="utf-8", newline="\n") as source_file:
    source_file.write(platform_source)
env.Append(CPPPATH=[
    env.subst("$PROJECT_INCLUDE_DIR"),
    join(microlink, "include"),
    join(microlink, "src"),
    join(wireguard, "src"),
    join(wireguard, "src", "crypto", "refc"),
])
env.Append(CPPDEFINES=["WIREGUARD_CRYPTO_REFC"])
env.BuildSources(
    join(env.subst("$BUILD_DIR"), "microlink"),
    join(microlink, "src"),
    src_filter=[
        "+<microlink.c>", "+<ml_coord.c>", "+<ml_derp.c>", "+<ml_net_io.c>",
        "+<ml_wg_mgr.c>", "+<ml_stun.c>", "+<ml_noise.c>", "+<ml_h2.c>",
        "+<ml_udp.c>", "+<ml_tcp.c>", "+<ml_peer_nvs.c>", "+<nacl_box.c>",
        "+<x25519.c>", "+<ml_zerocopy.c>",
    ],
)
env.BuildSources(
    join(env.subst("$BUILD_DIR"), "wireguard_lwip"),
    join(wireguard, "src"),
    src_filter=[
        "+<wireguard.c>", "+<wireguardif.c>", "+<crypto.c>",
        "+<wireguard-platform-esp32.c>", "+<crypto/refc/blake2s.c>",
        "+<crypto/refc/chacha20.c>", "+<crypto/refc/chacha20poly1305.c>",
        "+<crypto/refc/poly1305-donna.c>",
    ],
)
