# hakoniwa-core-cpp

[![Hakoniwa-core-cpp](https://github.com/toppers/hakoniwa-core-cpp/actions/workflows/build_and_test.yml/badge.svg?branch=main)](https://github.com/toppers/hakoniwa-core-cpp/actions/workflows/build_and_test.yml)

Hakoniwa-core-cpp は **シミュレーションハブの本体** となるコアライブラリです。共有メモリを利用したスタック構造により、アセット間で高速な PDU データ通信と時刻同期を実現しています。

## Documentation
- [API Specification](API_SPEC.md)
- [Shared Memory Layout](SHARED_MEMORY_SPEC.md)

## Stack Overview
1. Shared memory layer manages master data and PDU buffers.
2. Controller interfaces (master / asset / event) provide the public API.
3. Sample processes under `sample/` demonstrate typical usage.

## Simulation Flow
The following sequence diagram outlines how each component interacts during a
typical run. PlantUML can be used to render the diagram.

```plantuml
@startuml
actor "HakoCommand" as Cmd
participant "HakoMaster" as Master
participant "HakoAsset" as Asset
participant "IHakoMasterController" as MasterCtrl
participant "IHakoAssetController" as AssetCtrl
participant "IHakoSimulationEventController" as SimCtrl

Cmd -> Master: hako::init()
Master -> Master: hako::create_master() --> MasterCtrl
MasterCtrl -> MasterCtrl: set_config_simtime()

Asset -> Asset: hako::create_asset_controller() --> AssetCtrl
AssetCtrl -> AssetCtrl: create_pdu_lchannel()
AssetCtrl -> AssetCtrl: asset_register()

Cmd -> Cmd: hako::get_simevent_controller() --> SimCtrl
Cmd -> SimCtrl: start()

loop シミュレーション中
    Master -> MasterCtrl: execute()
    Asset -> AssetCtrl: is_simulation_mode()
    Asset -> AssetCtrl: notify_simtime()
    Asset -> AssetCtrl: read_pdu()/write_pdu()
end

Cmd -> SimCtrl: stop()
Cmd -> Cmd: hako::destroy()
Master -> Master: hako::destroy()
Asset -> Asset: hako::destroy()
@enduml
```

## Required
- If Using Google Test
  - sudo apt-get install libgtest-dev
