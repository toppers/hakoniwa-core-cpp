# Hakoniwa 共有メモリデータ構造仕様

Hakoniwaコアライブラリでは、マスターと各アセット間で共有メモリを介して状態やPDUデータを共有します。本ドキュメントではそのレイアウトを示します。

## 共有メモリID

`hako_config.hpp` で以下のIDが定義されています。

- `HAKO_SHARED_MEMORY_ID_0` : マスター情報(`HakoMasterDataType`)
- `HAKO_SHARED_MEMORY_ID_1` : PDUデータ領域(`HakoPduData`)
- `HAKO_SHARED_MEMORY_ID_2` : 拡張用

## HakoMasterDataType

マスター用共有メモリ( `HAKO_SHARED_MEMORY_ID_0` )に配置される構造体です。

```cpp
struct HakoMasterDataType {
    pid_type                master_pid;
    HakoSimulationStateType state;
    HakoTimeSetType         time_usec;
    uint32_t                asset_num;
    HakoAssetEntryType      assets[HAKO_DATA_MAX_ASSET_NUM];
    HakoAssetEntryEventType assets_ev[HAKO_DATA_MAX_ASSET_NUM];
    HakoPduMetaDataType     pdu_meta_data;
    HakoLogDataType         log;
};
```

### フィールド概要

- **master_pid** : マスタープロセスのPID
- **state** : シミュレーション状態
- **time_usec** : `max_delay` / `delta` / `current` を保持する時刻情報
- **asset_num** : 登録済みアセット数
- **assets[]** : アセットの静的情報(ID, 名前, 種別, コールバック)
- **assets_ev[]** : アセットの動的情報(PID, イベント, ハートビートなど)
- **pdu_meta_data** : PDUチャンネル管理情報(後述)
- **log** : 共有メモリ上のリングバッファログ

## HakoPduMetaDataType

`HakoMasterDataType` 内で使用され、PDUデータ( `HAKO_SHARED_MEMORY_ID_1` )の配置やアクセス状態を管理します。

```cpp
struct HakoPduMetaDataType {
    HakoTimeModeType    mode;
    HakoAssetIdType     asset_num;
    HakoAssetIdType     pdu_sync_asset_id;
    bool                asset_pdu_check_status[HAKO_DATA_MAX_ASSET_NUM];
    int32_t             channel_num;
    bool                is_dirty[HAKO_PDU_CHANNEL_MAX];
    bool                is_rbusy[HAKO_DATA_MAX_ASSET_NUM][HAKO_PDU_CHANNEL_MAX];
    bool                is_rbusy_for_external[HAKO_PDU_CHANNEL_MAX];
    bool                is_wbusy[HAKO_PDU_CHANNEL_MAX];
    HakoPduChannelType  channel[HAKO_PDU_CHANNEL_MAX];
    uint32_t            pdu_read_version[HAKO_DATA_MAX_ASSET_NUM][HAKO_PDU_CHANNEL_MAX];
    uint32_t            pdu_write_version[HAKO_PDU_CHANNEL_MAX];
    HakoPduChannelMapType channel_map[HAKO_PDU_CHANNEL_MAX];
};
```

各フィールドの役割は以下の通りです。

- **mode** : マスター駆動かアセット駆動かを示すモード
- **asset_num** : PDU同期対象となるアセット数
- **pdu_sync_asset_id** : 同期に利用するアセットID
- **asset_pdu_check_status[]** : アセット毎のPDU送信確認フラグ
- **channel_num** : 使用しているPDUチャネル数
- **is_dirty[]** : チャネル毎の更新有無
- **is_rbusy[][] / is_wbusy[]** : 読み書き中フラグ
- **channel[]** : 各チャネルの `offset` と `size`
- **pdu_read_version[][] / pdu_write_version[]** : 版数カウンタによる同期
- **channel_map[]** : 論理チャネル管理用マッピング

## PDUデータ領域

実際のPDUバイト列は `HAKO_SHARED_MEMORY_ID_1` に確保されます。`HakoPduData::create()` で生成され、`pdu_meta_data.channel[i].offset` / `size` に従って連続配置されます。

```
offset = channel[i].offset
size   = channel[i].size
pdu_memory[offset .. offset + size - 1] がチャネル i のデータ
```

リセット時には`HakoPduData::reset()`により初期化されます。読み書きAPIは`is_rbusy`や`is_wbusy`といったフラグを参照しながら排他制御を行います。

## ログバッファ

`HakoMasterDataType`末尾の`HakoLogDataType`は固定長のログ領域であり、`HAKO_LOGGER_LOGNUM`件のエントリをリングバッファ方式で保持します。

## 参考

- `src/hako/data/hako_base_data.hpp`
- `src/hako/data/hako_master_data.hpp`
- `src/hako/data/hako_pdu_data.hpp`
