# Library reader

House Cat Library provides a small, control-friendly public-domain catalog:

- *Alice's Adventures in Wonderland* — Lewis Carroll, Project Gutenberg #11
- *Pride and Prejudice* — Jane Austen, Project Gutenberg #1342
- *The Adventures of Sherlock Holmes* — Arthur Conan Doyle, Project Gutenberg #1661

Use Rocker Up/Down to choose a title and click to download it. Once the reader opens, Up/Down changes pages, Select returns to the catalog, and Back returns to the catalog or main menu.

The current page is automatically bookmarked in NVS. If the selected title is
still cached in LittleFS, the catalog shows `CLICK RESUME` and opens the saved
page without downloading again. The cache and bookmark survive a reboot.

Books are fetched one at a time from the `gutenberg.pglaf.org` Project Gutenberg mirror, stored in the 2 MiB `littlefs` partition, stripped of the Gutenberg header/footer for reading, and indexed locally into approximately 45-character large-type e-paper pages. The last downloaded text remains on flash and stays readable if Wi-Fi drops during the reading session. Downloading another title replaces it.

The firmware deliberately uses a three-title local catalog rather than crawling `gutenberg.org`. Project Gutenberg says its main human-facing site must not be automated and directs automated downloads to its mirrors and offline feeds:

- https://www.gutenberg.org/policy/robot_access.html
- https://www.gutenberg.org/ebooks/offline_catalogs.html

Project Gutenberg warns that copyright status can differ outside the United States. The curated titles are requested only from entries identified as public domain in the United States; deployments elsewhere must evaluate local law.

The download contains no credentials or private data. TLS is encrypted but currently uses the ESP32 insecure-certificate mode because this firmware does not ship a CA bundle. A future release should add certificate validation before accepting arbitrary or user-supplied content endpoints.
