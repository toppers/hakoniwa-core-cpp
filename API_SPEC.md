# Hakoniwa C++ Core API仕様書

このドキュメントでは、本リポジトリで提供されるC++向けHakoniwaコアライブラリのAPI概要を説明します。

## 基本
- ヘッダ `hako.hpp` をインクルードすることで各種機能を利用できます。
- 名前空間は `hako` を利用します。

## 初期化・終了
| 関数 | 説明 |
| --- | --- |
| `bool hako::init()` | ライブラリの初期化を行います。共有メモリの準備などを実施します。|
| `void hako::destroy()` | `init()` で確保したリソースを解放します。|

## コントローラクラス生成
| 関数 | 説明 |
| --- | --- |
| `std::shared_ptr<IHakoMasterController> hako::create_master()` | マスター制御用オブジェクトを生成します。|
| `std::shared_ptr<IHakoAssetController> hako::create_asset_controller()` | アセット制御用オブジェクトを生成します。|
| `std::shared_ptr<IHakoSimulationEventController> hako::get_simevent_controller()` | シミュレーションイベント制御オブジェクトを取得します。|

## IHakoMasterController
`hako_master.hpp` で定義されるインタフェースです。主なメンバーは以下の通りです。

| メソッド | 説明 |
| --- | --- |
| `bool execute()` | シミュレーションを1ステップ実行します。|
| `void set_config_simtime(HakoTimeType max_delay_time_usec, HakoTimeType delta_time_usec)` | シミュレーション時間の設定を行います。|
| `HakoTimeType get_max_deltay_time_usec()` | 設定された最大遅延時間(usec)を取得します。|
| `HakoTimeType get_delta_time_usec()` | 時間刻み(usec)を取得します。|

## IHakoAssetController
`hako_asset.hpp` で定義されるインタフェースです。主なAPIは以下の通りです。

- アセット管理
  - `bool asset_register(const std::string& name, AssetCallbackType& callbacks)`
  - `bool asset_register_polling(const std::string& name)`
  - `HakoSimulationAssetEventType asset_get_event(const std::string& name)`
  - `bool asset_unregister(const std::string& name)`
  - `void notify_simtime(const std::string& name, HakoTimeType simtime)`
  - `HakoTimeType get_worldtime()`

- フィードバックイベント
  - `bool start_feedback(const std::string& asset_name, bool isOk)`
  - `bool stop_feedback(const std::string& asset_name, bool isOk)`
  - `bool reset_feedback(const std::string& asset_name, bool isOk)`

- PDU通信
  - `bool create_pdu_channel(HakoPduChannelIdType channel_id, size_t pdu_size)`
  - `bool create_pdu_lchannel(const std::string& robo_name, HakoPduChannelIdType channel_id, size_t pdu_size)`
  - `HakoPduChannelIdType get_pdu_channel(const std::string& robo_name, HakoPduChannelIdType channel_id)`
  - `bool is_pdu_dirty(const std::string& asset_name, const std::string& robo_name, HakoPduChannelIdType channel_id)`
  - `bool write_pdu(const std::string& asset_name, const std::string& robo_name, HakoPduChannelIdType channel_id, const char* pdu_data, size_t len)`
  - `bool write_pdu_for_external(const std::string& robo_name, HakoPduChannelIdType channel_id, const char* pdu_data, size_t len)`
  - `bool read_pdu(const std::string& asset_name, const std::string& robo_name, HakoPduChannelIdType channel_id, char* pdu_data, size_t len)`
  - `bool read_pdu_for_external(const std::string& robo_name, HakoPduChannelIdType channel_id, char* pdu_data, size_t len)`
  - `void notify_read_pdu_done(const std::string& asset_name)`
  - `void notify_write_pdu_done(const std::string& asset_name)`
  - `bool is_pdu_sync_mode(const std::string& asset_name)`
  - `bool is_simulation_mode()`
  - `bool is_pdu_created()`
  - `bool write_pdu_nolock(const std::string& robo_name, HakoPduChannelIdType channel_id, const char* pdu_data, size_t len)`
  - `bool read_pdu_nolock(const std::string& robo_name, HakoPduChannelIdType channel_id, char* pdu_data, size_t len)`

## IHakoSimulationEventController
`hako_simevent.hpp` で定義されるインタフェースです。

| メソッド | 説明 |
| --- | --- |
| `HakoSimulationStateType state()` | 現在のシミュレーション状態を取得します。|
| `bool start()` | シミュレーション開始イベントを発行します。|
| `bool stop()` | シミュレーション停止イベントを発行します。|
| `bool reset()` | シミュレーションリセットイベントを発行します。|
| `bool assets(std::vector<std::shared_ptr<std::string>>& asset_list)` | 登録済みアセット名一覧を取得します。|
| `HakoPduChannelIdType get_pdu_channel(const std::string& asset_name, HakoPduChannelIdType channel_id)` | 指定資産のPDUチャンネルIDを取得します。|
| `bool write_pdu(HakoPduChannelIdType channel_id, const char* pdu_data, size_t len)` | PDUデータを書き込みます。|
| `bool read_pdu(HakoPduChannelIdType channel_id, char* pdu_data, size_t len)` | PDUデータを読み込みます。|
| `size_t pdu_size(HakoPduChannelIdType channel_id)` | 指定チャンネルのPDUサイズを取得します。|
| `void print_master_data()` | マスターが保持する各種情報を標準出力へ表示します。|
| `void print_memory_log()` | 共有メモリ上に記録されたログを表示します。|

## 主要データ型
`types/hako_types.hpp` で定義される主な型は以下の通りです。

- `HakoTimeType` : 時刻（マイクロ秒）を表す 64bit 整数。
- `HakoAssetIdType` : アセットIDを表す 32bit 整数。
- `HakoPduChannelIdType` : PDUチャネルIDを表す 32bit 整数。
- `HakoSimulationStateType` : シミュレーション状態を示す列挙体。
- `HakoSimulationAssetEventType` : アセット向けイベント種別を示す列挙体。
- `HakoFixedStringType` : 固定長文字列構造体。
- `AssetCallbackType` : アセットの start/stop/reset コールバックを保持する構造体。

## ログ出力
`hako_log.hpp` では以下のマクロが提供されています。

- `HAKO_LOG_INFO(...)`
- `HAKO_LOG_WARN(...)`
- `HAKO_LOG_ERROR(...)`

いずれも内部で `hako::log::add()` を呼び出し、ログを共有メモリへ追加します。ロック区間からの呼び出しはデッドロックの恐れがあるため注意が必要です。

## 定数
設定値は `config/hako_config.hpp` に定義されています。主な定数は以下の通りです。

- `HAKO_FIXED_STRLEN_MAX` : 固定長文字列の最大長。
- `HAKO_DATA_MAX_ASSET_NUM` : 登録可能なアセット数。
- `HAKO_PDU_CHANNEL_MAX` : PDUチャンネル上限数。
- `HAKO_ASSET_TIMEOUT_USEC` : アセットのタイムアウト値。

## 参考サンプル
`sample/base-procs` 配下に基本的なマスター・アセット・コマンドツールの実装例があります。実際の利用方法についてはこれらのサンプルを参照してください。

