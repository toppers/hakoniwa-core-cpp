#ifndef _HAKO_MASTER_DATA_HPP_
#define _HAKO_MASTER_DATA_HPP_

#include "data/hako_base_data.hpp"
#include "utils/hako_shared_memory.hpp"
#include "utils/hako_string.hpp"
#include "utils/hako_assert.hpp"
#include "utils/hako_clock.hpp"
#include "utils/hako_sem.hpp"
#include "core/context/hako_context.hpp"
#include <string.h>

namespace hako::data {

    typedef struct {
        pid_t                   master_pid;
        HakoSimulationStateType state;
        HakoTimeSetType         time_usec;
        uint32_t                asset_num;
        HakoAssetEntryType      assets[HAKO_DATA_MAX_ASSET_NUM];
        HakoAssetEntryEventType assets_ev[HAKO_DATA_MAX_ASSET_NUM];
    } HakoMasterDataType;

    class HakoMasterData {
    public:
        HakoMasterData()
        {
        }
        virtual ~HakoMasterData()
        {
        }
        void init()
        {
            this->shmp_ = std::make_shared<hako::utils::HakoSharedMemory>();
            auto shmid = this->shmp_->create_memory(HAKO_SHARED_MEMORY_ID_0, sizeof(HakoMasterDataType));
            HAKO_ASSERT(shmid >= 0);
            void *datap = this->shmp_->lock_memory(HAKO_SHARED_MEMORY_ID_0);
            this->master_datap_ = static_cast<HakoMasterDataType*>(datap);
            {
                hako::core::context::HakoContext context;
                memset(this->master_datap_, 0, sizeof(HakoMasterDataType));
                this->master_datap_->master_pid = context.get_pid();
            }
            this->shmp_->unlock_memory(HAKO_SHARED_MEMORY_ID_0);
        }
        pid_t get_master_pid()
        {
            return this->master_datap_->master_pid;
        }
        int32_t get_semid()
        {
            return this->shmp_->get_semid(HAKO_SHARED_MEMORY_ID_0);
        }

        bool load()
        {
            if (this->shmp_ != nullptr) {
                return true;
            }
            this->shmp_ = std::make_shared<hako::utils::HakoSharedMemory>();
            void *datap = this->shmp_->load_memory(HAKO_SHARED_MEMORY_ID_0, sizeof(HakoMasterDataType));
            if (datap == nullptr) {
                return false;
            }
            this->master_datap_ = static_cast<HakoMasterDataType*>(datap);
            HAKO_ASSERT((this->shmp_ != nullptr) && (this->master_datap_ != nullptr));
            return true;
        }

        void destroy()
        {
            if (this->shmp_ != nullptr) {
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
        }
        void unlock()
        {
            HAKO_ASSERT((this->shmp_ != nullptr) && (this->master_datap_ != nullptr));
            (void)this->shmp_->unlock_memory(HAKO_SHARED_MEMORY_ID_0);
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
        HakoAssetIdType alloc_asset(const std::string &name, HakoAssetType type, AssetCallbackType &callback)
        {
            hako::core::context::HakoContext context;

            HAKO_ASSERT((this->shmp_ != nullptr) && (this->master_datap_ != nullptr));
            if (type == hako::data::HakoAssetType::HakoAsset_Unknown) {
                return -1;
            }
            if (name.length() > HAKO_FIXED_STRLEN_MAX) {
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
                        this->master_datap_->assets[i].callback = callback;
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
            this->lock();
            for (int i = 0; i < HAKO_DATA_MAX_ASSET_NUM; i++) {
                HakoAssetEntryType &entry = this->master_datap_->assets[i];
                if (entry.type == hako::data::HakoAssetType::HakoAsset_Unknown) {
                    continue;
                }                
                asset_times.push_back(this->master_datap_->assets_ev[i].ctime);
            }
            this->unlock();
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

    private:
        std::shared_ptr<hako::utils::HakoSharedMemory>  shmp_;
        HakoMasterDataType *master_datap_ = nullptr;
    };
}

#endif /* _HAKO_MASTER_DATA_HPP_ */