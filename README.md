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
typical run.

```mermaid
sequenceDiagram
    actor Cmd as HakoCommand
    participant Master as HakoMaster
    participant Asset as HakoAsset
    participant MasterData as "Master Data"
    participant PduData as "PDU Data"

    Cmd->>Master: hako::init()
    activate Master
    Master->>MasterData: create()
    activate MasterData
    note over Master,MasterData: Masterデータ領域を確保

    Asset->>Master: asset_register()
    Master->>MasterData: アセット情報を登録
    Asset->>MasterData: create_pdu_lchannel()
    note left of Asset: PDUチャネルを定義

    Cmd->>Master: start()
    note right of Master: シミュレーションをRunnable状態へ
    Master->>MasterData: create_pdu_data()
    MasterData->>PduData: create()
    activate PduData
    note over MasterData,PduData: PDUデータ領域を確保

    loop シミュレーション実行中
        Asset-->>PduData: read_pdu() / write_pdu()
    end

    Cmd->>Master: stop()
    note right of Master: シミュレーションを停止

    Cmd->>Master: hako::destroy()
    Master->>MasterData: destroy_pdu_data()
    PduData-->>MasterData: destroyed
    deactivate PduData
    note over Master,MasterData: PDUデータ領域を破棄

    Master-->>MasterData: destroyed
    deactivate MasterData
    note over Master,MasterData: Masterデータ領域を破棄
    deactivate Master

```



## Required
- If Using Google Test
  - sudo apt-get install libgtest-dev
