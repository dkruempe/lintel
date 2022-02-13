# TODOs

- [ ] general: automatic create conan package.
- [ ] general: find general std::size_t serialize solution instead of limiting it to __APPLE__
- [ ] http: implement partly better exception handling
- [ ] http: support of paging
- [ ] db: change std::string implementations to std::string_view for better performance
- [ ] db: switch partly to constexpr implementation ?!
- [ ] property: cleanup entries if default value is same no repository in shadow mode at startup
- [ ] shm: fix automatic increase feature, bc. currently free_memory is always fixed