#ifndef _HAKO_MASTER_DATA_HPP_
#define _HAKO_MASTER_DATA_HPP_

#include "data/hako_base_data.hpp"
#include "utils/hako_share/hako_shared_memory_factory.hpp"
#include "utils/hako_string.hpp"
#include "utils/hako_assert.hpp"
#include "utils/hako_clock.hpp"
#include "core/context/hako_context.hpp"
#include "data/hako_pdu_data.hpp"
#ifdef HAKO_CORE_EXTENSION
#include "hako_extension.hpp"
#endif
#include <string.h>

namespace hako::data {

    typedef struct {
        pid_type                master_pid;
        HakoSimulationStateType state;
        HakoTimeSetType         time_usec;
        uint32_t                asset_num;
        HakoAssetEntryType      assets[HAKO_DATA_MAX_ASSET_NUM];
        HakoAssetEntryEventType assets_ev[HAKO_DATA_MAX_ASSET_NUM];

        /*
         * PDU MetaData
         */
        HakoPduMetaDataType     pdu_meta_data;

        /*
         * MEMORY LOG
         */
        HakoLogDataType log;
    } HakoMasterDataType;

    class HakoMasterData {
    public:
        HakoMasterData()
        {
            this->shm_type_ = "shm";
        }
        virtual ~HakoMasterData()
        {
        }
        std::string get_shm_type()
        {
            return this->shm_type_;
        }
        void init(const std::string& type="shm")
        {
            if (type != "shm")
            {
                this->shm_type_ = type;
            }
            this->shmp_ = hako::utils::hako_shared_memory_create(this->shm_type_);
            auto shmid = this->shmp_->create_memory(HAKO_SHARED_MEMORY_ID_0, sizeof(HakoMasterDataType));
            HAKO_ASSERT(shmid >= 0);
            (void)shmid;
            void *datap = this->shmp_->lock_memory(HAKO_SHARED_MEMORY_ID_0);
            this->master_datap_ = static_cast<HakoMasterDataType*>(datap);
            {
                hako::core::context::HakoContext context;
                memset(this->master_datap_, 0, sizeof(HakoMasterDataType));
                this->master_datap_->master_pid = context.get_pid();
            }
            this->shmp_->unlock_memory(HAKO_SHARED_MEMORY_ID_0);
            this->pdu_datap_ = std::make_shared<HakoPduData>(&this->master_datap_->pdu_meta_data, this->shmp_, this->get_shm_type());
        }
        pid_type get_master_pid()
        {
            return this->master_datap_->master_pid;
        }
        int32_t get_semid()
        {
            return this->shmp_->get_semid(HAKO_SHARED_MEMORY_ID_0);
        }

        bool load(const std::string& type="shm")
        {
            if (this->shmp_ != nullptr) {
                return true;
            }
            if (type != "shm")
            {
                this->shm_type_ = type;
            }
            this->shmp_ = hako::utils::hako_shared_memory_create(this->get_shm_type());
            void *datap = this->shmp_->load_memory(HAKO_SHARED_MEMORY_ID_0, sizeof(HakoMasterDataType));
            if (datap == nullptr) {
                return false;
            }
            //printf("master load:addr=%p\n", datap);
            this->master_datap_ = static_cast<HakoMasterDataType*>(datap);
            HAKO_ASSERT((this->shmp_ != nullptr) && (this->master_datap_ != nullptr));
            this->pdu_datap_ = std::make_shared<HakoPduData>(&this->master_datap_->pdu_meta_data, this->shmp_, this->get_shm_type());
            return true;
        }

        void destroy()
        {
            if (this->shmp_ != nullptr) {
#ifdef HAKO_CORE_EXTENSION
                if (this->master_ext_ != nullptr) {
                    this->master_ext_->on_pdu_data_destroy();
                }
#endif
                this->pdu_datap_->destroy();
                this->shmp_->destroy_memory(HAKO_SHARED_MEMORY_ID_0);
                this->master_datap_ = nullptr;
                this->shmp_ = nullptr;
            }
        }

        /*
         * data exclusive lock APIs
         */
        void lock()
        {
            HAKO_ASSERT((this->shmp_ != nullptr) && (this->master_datap_ != nullptr));
            (void)this->shmp_->lock_memory(HAKO_SHARED_MEMORY_ID_0);
            this->is_master_locked = true;
        }
        void unlock()
        {
            HAKO_ASSERT((this->shmp_ != nullptr) && (this->master_datap_ != nullptr));
            this->is_master_locked = false;
            (void)this->shmp_->unlock_memory(HAKO_SHARED_MEMORY_ID_0);
        }
        bool is_locked()
        {
            return this->is_master_locked;
        }
        /*
         * Time APIs
         */
        HakoTimeSetType get_time()
        {
            HAKO_ASSERT((this->shmp_ != nullptr) && (this->master_datap_ != nullptr));
            this->lock();
            HakoTimeSetType timeset = this->master_datap_->time_usec;
            this->unlock();
            return timeset;
        }
        HakoTimeSetType& ref_time_nolock()
        {
            HAKO_ASSERT((this->shmp_ != nullptr) && (this->master_datap_ != nullptr));
            HakoTimeSetType &timeset = this->master_datap_->time_usec;
            return timeset;
        }
        void update_asset_time(HakoAssetIdType id)
        {
            if ((id >= 0) && (id < HAKO_DATA_MAX_ASSET_NUM)) {
                if (this->master_datap_->assets[id].type != hako::data::HakoAssetType::HakoAsset_Unknown) {
                    this->master_datap_->assets_ev[id].update_time = hako_get_clock();
                }
            }
            return;
        }
        HakoSimulationStateType& ref_state_nolock()
        {
            return this->master_datap_->state;
        }
        /*
         * Assets APIs
         */        
        HakoAssetIdType alloc_asset(const std::string &name, HakoAssetType type, AssetCallbackType *callback)
        {
            hako::core::context::HakoContext context;

            HAKO_ASSERT((this->shmp_ != nullptr) && (this->master_datap_ != nullptr));
            if (type == hako::data::HakoAssetType::HakoAsset_Unknown) {
                HAKO_LOG_ERROR("alloc_asset type is unknown");
                return -1;
            }
            if (name.length() > HAKO_FIXED_STRLEN_MAX) {
                HAKO_LOG_ERROR("alloc_asset name len is big: %d", name.length());
                return -1;
            }
            HakoAssetIdType id = -1;
            this->lock();
            if (this->master_datap_->state == HakoSim_Stopped &&
                (this->get_asset_nolock(name) == nullptr) &&
                (this->master_datap_->asset_num < HAKO_DATA_MAX_ASSET_NUM)) {
                for (int i = 0; i < HAKO_DATA_MAX_ASSET_NUM; i++) {
                    if (this->master_datap_->assets[i].type == hako::data::HakoAssetType::HakoAsset_Unknown) {
                        this->master_datap_->assets[i].id = i;
                        this->master_datap_->assets[i].type = type;
                        if (type == hako::data::HakoAssetType::HakoAsset_Outside) {

                        }
                        else {
                            this->master_datap_->assets_ev[i].semid = -1;
                        }
                        if (callback != nullptr) {
                            this->master_datap_->assets[i].callback = *callback;
                        }
                        else {
                            this->master_datap_->assets[i].callback.reset = nullptr;
                            this->master_datap_->assets[i].callback.start = nullptr;
                            this->master_datap_->assets[i].callback.stop = nullptr;
                        }
                        this->master_datap_->assets_ev[i].pid = context.get_pid();
                        this->master_datap_->assets_ev[i].ctime = 0ULL;
                        this->update_asset_time(i);
                        hako::utils::hako_string2fixed(name, this->master_datap_->assets[i].name);
                        id = i;
                        this->master_datap_->asset_num++;
                        break;
                    }
                }
            }
            else {
                /* nothing to do */
            }
            this->unlock();
            return id;
        }
        HakoAssetIdType get_asset_id()
        {
            HakoAssetIdType id = -1;
            hako::core::context::HakoContext context;
            pid_type pid = context.get_pid();
            for (int i = 0; i < HAKO_DATA_MAX_ASSET_NUM; i++) {
                if (this->master_datap_->assets[i].type != HakoAsset_Unknown) {
                    if (this->master_datap_->assets_ev[i].pid == pid) {
                        id = i;
                        break;
                    }
                }
            }
            return id;
        }
        bool free_asset(const std::string &name)
        {
            bool ret = false;
            this->lock();
            if ((this->master_datap_->state == HakoSim_Stopped) || (this->master_datap_->state == HakoSim_Error)) {
                HakoAssetEntryType *entry = this->get_asset_nolock(name);
                if (entry != nullptr) {
                    entry->type = hako::data::HakoAssetType::HakoAsset_Unknown;
                    this->master_datap_->asset_num--;
                    ret = true;
                }
            }
            this->unlock();
            return ret;
        }
        HakoAssetEntryType *get_asset(HakoAssetIdType id)
        {
            if ((id >= 0) && (id < HAKO_DATA_MAX_ASSET_NUM)) {
                if (this->master_datap_->assets[id].type != hako::data::HakoAssetType::HakoAsset_Unknown) {
                    return &this->master_datap_->assets[id];
                }
            }
            return nullptr;
        }
        HakoAssetEntryEventType *get_asset_event_nolock(HakoAssetIdType id)
        {
            if ((id >= 0) && (id < HAKO_DATA_MAX_ASSET_NUM)) {
                if (this->master_datap_->assets[id].type != hako::data::HakoAssetType::HakoAsset_Unknown) {
                    return &this->master_datap_->assets_ev[id];
                }
            }
            return nullptr;
        }

        HakoAssetEntryType *get_asset(const std::string &name)
        {
            HakoAssetEntryType *entry;
            this->lock();
            entry = this->get_asset_nolock(name);
            this->unlock();
            return entry;
        }
        bool is_all_feedback_nolock()
        {
            bool ret = true;
            for (int i = 0; i < HAKO_DATA_MAX_ASSET_NUM; i++) {
                HakoAssetEntryType &entry = this->master_datap_->assets[i];
                if (entry.type == hako::data::HakoAssetType::HakoAsset_Unknown) {
                    continue;
                }
                else if (this->master_datap_->assets_ev[i].event_feedback == false) {
                    ret = false;
                    break;
                }
            }
            return ret;
        }
        bool is_asset_timeout_nolock(HakoAssetIdType id)
        {
            if ((id >= 0) && (id < HAKO_DATA_MAX_ASSET_NUM)) {
                return hako_clock_is_timeout(this->master_datap_->assets_ev[id].update_time, HAKO_ASSET_TIMEOUT_USEC);
            }
            return false;
        }
        HakoAssetEntryType *get_asset_nolock(const std::string &name)
        {
            for (int i = 0; i < HAKO_DATA_MAX_ASSET_NUM; i++) {
                HakoAssetEntryType &entry = this->master_datap_->assets[i];
                if (entry.type == hako::data::HakoAssetType::HakoAsset_Unknown) {
                    continue;
                }
                else if (entry.name.len != name.length()) {
                    continue;
                }
                else if (strncmp(entry.name.data, name.c_str(), entry.name.len) != 0) {
                    continue;
                }
                return &entry;
            }
            return nullptr;
        }
        void get_asset_times(std::vector<HakoTimeType> & asset_times)
        {
            //this->lock();
            for (int i = 0; i < HAKO_DATA_MAX_ASSET_NUM; i++) {
                HakoAssetEntryType &entry = this->master_datap_->assets[i];
                if (entry.type == hako::data::HakoAssetType::HakoAsset_Unknown) {
                    continue;
                }                
                asset_times.push_back(this->master_datap_->assets_ev[i].ctime);
            }
            //this->unlock();
        }
        void get_asset_names(std::vector<std::shared_ptr<std::string>> &asset_lists)
        {
            this->lock();
            for (int i = 0; i < HAKO_DATA_MAX_ASSET_NUM; i++) {
                HakoAssetEntryType &entry = this->master_datap_->assets[i];
                if (entry.type == hako::data::HakoAssetType::HakoAsset_Unknown) {
                    continue;
                }
                std::shared_ptr<std::string> asset_name = hako::utils::hako_fixed2string(entry.name);
                asset_lists.push_back(asset_name);
            }
            this->unlock();
        }
        std::shared_ptr<HakoPduData> get_pdu_data()
        {
            return this->pdu_datap_;
        }
        void destroy_pdu_data()
        {
#ifdef HAKO_CORE_EXTENSION
            if (this->master_ext_ != nullptr) {
                this->master_ext_->on_pdu_data_reset();
            }
#endif
            this->pdu_datap_->reset();
            //this->pdu_datap_->destroy();
        }
        void create_pdu_data()
        {
            this->pdu_datap_->create(this->master_datap_->asset_num);
        }
        void print_master_data()
        {
            std::cout << "master_pid = " << this->master_datap_->master_pid << std::endl;
            std::cout << "state = " << this->master_datap_->state << std::endl;
            std::cout << "time_usec = " << this->master_datap_->time_usec.current << std::endl;
            std::cout << "asset_num = " << this->master_datap_->asset_num << std::endl;
            for (int i = 0; i < HAKO_DATA_MAX_ASSET_NUM; i++) {
                std::cout << "assets[" << i << "].id = " << this->master_datap_->assets[i].id << std::endl;
                std::cout << "assets[" << i << "].type = " << this->master_datap_->assets[i].type << std::endl;
                if (this->master_datap_->assets[i].type != HakoAsset_Unknown) {
                    std::cout << "assets[" << i << "].name = " << std::string(this->master_datap_->assets[i].name.data) << std::endl;
                    std::cout << "assets_ev[" << i << "].pid = " << this->master_datap_->assets_ev[i].pid << std::endl;
                    std::cout << "assets_ev[" << i << "].ctime = " << this->master_datap_->assets_ev[i].ctime << std::endl;
                    std::cout << "assets_ev[" << i << "].update_time = " << this->master_datap_->assets_ev[i].update_time << std::endl;
                    std::cout << "assets_ev[" << i << "].event = " << this->master_datap_->assets_ev[i].event << std::endl;
                    std::cout << "assets_ev[" << i << "].event_feedback = " << this->master_datap_->assets_ev[i].event_feedback << std::endl;
                    //std::cout << "assets_ev[" << i << "].semid = " << this->master_datap_->assets_ev[i].semid << std::endl;
                }
            }

            std::cout << "pdu_meta_data.mode = " << this->master_datap_->pdu_meta_data.mode << std::endl;
            std::cout << "pdu_meta_data.asset_num = " << this->master_datap_->pdu_meta_data.asset_num << std::endl;
            std::cout << "pdu_meta_data.pdu_sync_asset_id = " << this->master_datap_->pdu_meta_data.pdu_sync_asset_id << std::endl;
            std::cout << "pdu_meta_data.channel_num = " << this->master_datap_->pdu_meta_data.channel_num << std::endl;
            for (int i = 0; i < HAKO_DATA_MAX_ASSET_NUM; i++) {
                if (this->master_datap_->assets[i].type != HakoAsset_Unknown) {
                    std::cout << "pdu_meta_data.asset_pdu_check_status[" << i << "] = " << this->master_datap_->pdu_meta_data.asset_pdu_check_status[i] << std::endl;
                    std::cout << "pdu_meta_data.asset_pdu_check_status[" << i << "] = " << this->master_datap_->pdu_meta_data.asset_pdu_check_status[i] << std::endl;
                }
            }
            for (int i = 0; i < this->master_datap_->pdu_meta_data.channel_num; i++) {
                std::cout << "pdu_meta_data.channel[" << i << "].offset = " << this->master_datap_->pdu_meta_data.channel[i].offset << std::endl;
                std::cout << "pdu_meta_data.channel[" << i << "].size = " << this->master_datap_->pdu_meta_data.channel[i].size << std::endl;
            }
            for (int i = 0; i < this->master_datap_->pdu_meta_data.channel_num; i++) {
                std::cout << "pdu_meta_data.channel_map[" << i << "].robo_name = " << std::string(this->master_datap_->pdu_meta_data.channel_map[i].robo_name.data) << std::endl;
                std::cout << "pdu_meta_data.channel_map[" << i << "].logical_channel_id = " << this->master_datap_->pdu_meta_data.channel_map[i].logical_channel_id << std::endl;
            }
        }
        void add_log(const char* format, va_list args)
        {
            HAKO_ASSERT(this->master_datap_ != nullptr);
            if (this->master_datap_->log.index >= HAKO_LOGGER_LOGNUM) {
                return;
            }
            this->lock();
            int index = this->master_datap_->log.index;
            this->master_datap_->log.index++;
            this->unlock();

            int written = vsnprintf(this->master_datap_->log.data[index], HAKO_LOGGER_ENTRYSIZE, format, args);
            if (written >= HAKO_LOGGER_ENTRYSIZE) {
                std::cerr << "Warning: Log message truncated to fit " << HAKO_LOGGER_ENTRYSIZE << " byte limit.\n";
            }
        }
        void add_log_internal(const char* level, const char* file, int line, const char* function, const char* format, va_list args)
        {
            char new_format[1024]; // 新しいフォーマットのためのバッファ
            snprintf(new_format, sizeof(new_format), "%s [%s:%d %s] %s", level, file, line, function, format);
            this->add_log(new_format, args);
        }
        void print_log()
        {
            HAKO_ASSERT(this->master_datap_ != nullptr);
            for (int i = 0; i < this->master_datap_->log.index; i++) {
                std::cout << std::string(this->master_datap_->log.data[i]) << std::endl;
            }
        }
#ifdef HAKO_CORE_EXTENSION
        void register_master_extension(std::shared_ptr<extension::IHakoMasterExtension> ext)
        {
            std::cout << "INFO: register_master_extension()" << std::endl;
            this->master_ext_ = ext;
        }
        void register_asset_extension(std::shared_ptr<extension::IHakoAssetExtension> aext)
        {
            std::cout << "INFO: register_asset_extension()" << std::endl;
            this->pdu_datap_->register_asset_extension(aext);
            this->pdu_datap_->register_master_extension(master_ext_);
        }
        std::shared_ptr<extension::IHakoMasterExtension> get_master_extension()
        {
            return this->master_ext_;
        }
        std::shared_ptr<extension::IHakoAssetExtension> get_asset_extension()
        {
            return this->pdu_datap_->get_asset_extension();
        }
#endif
    private:
        std::shared_ptr<hako::utils::HakoSharedMemory>  shmp_;
        HakoMasterDataType *master_datap_ = nullptr;
        std::shared_ptr<HakoPduData> pdu_datap_ = nullptr;
        std::string shm_type_;
        bool is_master_locked = false;
#ifdef HAKO_CORE_EXTENSION
        std::shared_ptr<extension::IHakoMasterExtension> master_ext_ = nullptr;
#endif
    };
}

#endif /* _HAKO_MASTER_DATA_HPP_ */