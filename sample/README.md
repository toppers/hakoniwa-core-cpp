# Samples

## Hakoniwa Base Architecture

- One Hako-Master Process
  - hako-time sync manager
  - hako-assets manager
- Multiple Hako-Asset Processes
  - hako-asset with simulation time
  - sync simulation time with other assets
    - hako-time is used for syncing
- Hako-Command
  - hakoniwa simulation controller
    - do start, stop, reset

## Sample programs
The reference implementations of above components are as follows.

- Hako-Master
  - sample/base-procs/hako-master
- Hako-Asset
  - sample/base-procs/hako-asset
- Hako-Command
  - sample/base-procs/hako-cmd


## How to make it work with sample programs 

1. Activate hako-master
2. Activate hako-asset with unique name in hakoniwa world.
3. Start simulation with hako-start command.

