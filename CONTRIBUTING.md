# Contributing

Thanks for helping House Cat. Small, focused pull requests are easiest to
review. Before opening one, run:

```bash
python tools/verify_repo.py
python tools/security_audit.py
cmake -S . -B build -G Ninja -DHOUSECAT_ENABLE_SANITIZERS=ON
cmake --build build
ctest --test-dir build --output-on-failure
python -m platformio run -e crowpanel_idf5
```

Never commit `include/secrets.h`, tokens, device credentials, private network
details, firmware dumps, or serial logs containing identifying information.
Use `include/secrets.example.h` for new configuration options and keep its
values inert. UI changes should update the native snapshots and include before
and after images in the pull request.

By contributing, you agree that your contribution is licensed under the MIT
license. Please follow the [Code of Conduct](CODE_OF_CONDUCT.md), report
security issues privately as described in [SECURITY.md](SECURITY.md), and add a
release-note entry for user-visible changes.
