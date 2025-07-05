# hakoniwa-core-cpp (English)

Hakoniwa-core-cpp is the **core library of the simulation hub**. It uses a shared memory stack to achieve fast PDU communication and time synchronization between assets.

## Documentation
- [API Specification](API_SPEC_EN.md)
- [Shared Memory Layout](SHARED_MEMORY_SPEC_EN.md)

## Stack Overview
1. Shared memory layer manages master data and PDU buffers.
2. Controller interfaces (master / asset / event) provide the public API.
3. Sample processes under `sample/` demonstrate typical usage.

## Simulation Flow
The following sequence diagram outlines how each component interacts during a typical run.

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
    note over Master,MasterData: allocate master data region

    Asset->>Master: asset_register()
    Master->>MasterData: register asset info
    Asset->>MasterData: create_pdu_lchannel()
    note left of Asset: define PDU channel

    Cmd->>Master: start()
    note right of Master: set simulation runnable
    Master->>MasterData: create_pdu_data()
    MasterData->>PduData: create()
    activate PduData
    note over MasterData,PduData: allocate PDU data region

    Master->>Asset: start_callback()
    Asset->>Master: start_feedback()

    loop while simulation running
        Asset-->>PduData: read_pdu() / write_pdu()
    end

    Cmd->>Master: stop()
    note right of Master: stop simulation
    Master->>Asset: stop_callback()
    Asset->>Master: stop_feedback()
```

## Required
- If Using Google Test
  - sudo apt-get install libgtest-dev
