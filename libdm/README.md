# libdm synchronization

This directory contains the C sources shared with the upstream `dmtools` project. To refresh the copies here with the maintained versions from the `codex/investigate-memory-management-issues-in-dmtools` branch of `https://github.com/ZhouQiangwei/dmtools`, run the helper script below once network access is available.

```
./update_from_dmtools.sh
```

The script fetches the files by name and replaces the local copies. If the environment cannot reach GitHub (for example, due to a restricted CONNECT tunnel), rerun the script when connectivity is restored.
