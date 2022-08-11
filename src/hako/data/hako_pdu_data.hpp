#ifndef _HAKO_PDU_DATA_HPP_
#define _HAKO_PDU_DATA_HPP_

#include "data/hako_base_data.hpp"
#include "utils/hako_shared_memory.hpp"
#include <string.h>
#include <iostream>

namespace hako::data {
    class HakoPduData {
    public:
        HakoPduData(HakoPduMetaDataType *pdu_meta_data, std::shared_ptr<hako::utils::HakoSharedMemory> master_shmp)
        {
            this->pdu_meta_data_ = pdu_meta_data;
            this->master_shmp_ = master_shmp;
            this->asset_shmp_ = std::make_shared<hako::utils::HakoSharedMemory>();
        }
        virtual ~HakoPduData()
        {
            this->pdu_meta_data_ = nullptr;
        }
        bool create_channel(HakoPduChannelIdType channel_id, size_t size)
        {
            bool ret = false;
            if (channel_id >= HAKO_PDU_CHANNEL_MAX) {
                return false;
            }
            this->master_shmp_->lock_memory(HAKO_SHARED_MEMORY_ID_0);
            if (this->pdu_meta_data_->channel[channel_id].size == 0) {
                this->pdu_meta_data_->channel[channel_id].size = size;
                std::cout << "create_channel: id=" << channel_id << " size=" << size << std::endl;
                this->pdu_meta_data_->channel_num++;
                ret = true;
            }
            this->master_shmp_->unlock_memory(HAKO_SHARED_MEMORY_ID_0);
            return ret;
        }
        bool is_pdu_dirty(HakoPduChannelIdType channel_id)
        {
            return this->pdu_meta_data_->is_dirty[channel_id];
        }

        bool is_pdu_wbusy(HakoPduChannelIdType channel_id)
        {
            return this->pdu_meta_data_->is_wbusy[channel_id];
        }
        bool is_pdu_rbusy(HakoPduChannelIdType channel_id)
        {
            return this->pdu_meta_data_->is_rbusy[channel_id];
        }
        void set_pdu_wbusy_status(HakoPduChannelIdType channel_id, bool busy_status)
        {
            this->pdu_meta_data_->is_wbusy[channel_id] = busy_status;
        }
        void set_pdu_rbusy_status(HakoPduChannelIdType channel_id, bool busy_status)
        {
            this->pdu_meta_data_->is_rbusy[channel_id] = busy_status;
        }
        /*
         * writers: only one!
         */
        bool write_pdu(HakoPduChannelIdType channel_id, const char *pdu_data, size_t len)
        {
            //printf("write_pdu: channel_id=%d len=%zu size=%zu\n", channel_id, len, this->pdu_meta_data_->channel[channel_id].size);
            if (channel_id >= HAKO_PDU_CHANNEL_MAX) {
                return false;
            }
            else if (len != this->pdu_meta_data_->channel[channel_id].size) {
                return false;
            }
            else if (this->pdu_ == nullptr) {
                this->load();
            }
            this->set_pdu_wbusy_status(channel_id, true);
            while (this->is_pdu_rbusy(channel_id)) {
                //usleep(1000); /* 1msec sleep */
            }
            int off = this->pdu_meta_data_->channel[channel_id].offset;
            memcpy(&this->pdu_[off], &pdu_data[0], len);
            this->pdu_meta_data_->is_dirty[channel_id] = true;
            this->set_pdu_wbusy_status(channel_id, false);
            return true;
        }

        /*
         * readers: only one!
         */
        bool read_pdu(HakoPduChannelIdType channel_id, char *pdu_data, size_t len)
        {
            if (channel_id >= HAKO_PDU_CHANNEL_MAX) {
                return false;
            }
            else if (len != this->pdu_meta_data_->channel[channel_id].size) {
                return false;
            }
            else if (this->pdu_ == nullptr) {
                this->load();
            }
            //READER wait until WRITER has done with rbusy flag=false 
            //in order to avoid deadlock situation for waiting each other...
            while (true) {
                this->set_pdu_rbusy_status(channel_id, true);
                if (this->is_pdu_wbusy(channel_id)) {
                    this->set_pdu_rbusy_status(channel_id, false);
                    //usleep(1000); /* 1msec sleep */
                    continue;
                }
                break;
            }
            int off = this->pdu_meta_data_->channel[channel_id].offset;
            memcpy(pdu_data, &this->pdu_[off], len);
            this->set_pdu_rbusy_status(channel_id, false);
            return true;
        }
        void notify_read_pdu_done(HakoAssetIdType asset_id)
        {
            if (asset_id >= HAKO_PDU_CHANNEL_MAX) {
                return;
            }
            this->pdu_meta_data_->asset_pdu_check_status[asset_id] = true;
            return;
        }
        void notify_write_pdu_done(HakoAssetIdType asset_id)
        {
            if (asset_id >= HAKO_PDU_CHANNEL_MAX) {
                return;
            }
            HakoAssetIdType next_id = asset_id + 1;
            if (next_id == this->pdu_meta_data_->asset_num) {
                next_id = 0;
            }
            this->pdu_meta_data_->asset_pdu_check_status[asset_id] = false;
            this->pdu_meta_data_->pdu_sync_asset_id = next_id;
            if (next_id == 0) {
                this->pdu_meta_data_->mode = HakoTimeMode_Master;
            }
            return;
        }
        bool is_pdu_sync_mode(HakoAssetIdType asset_id)
        {
            if (this->pdu_meta_data_->mode == HakoTimeMode_Master) {
                return false;
            }
            else if (this->pdu_meta_data_->pdu_sync_asset_id != asset_id) {
                return false;
            }
            return true;
        }
        bool is_simulation_mode()
        {
            if (this->pdu_meta_data_->mode == HakoTimeMode_Master) {
                return true;
            }
            return false;
        }
        /*
         * for master api
         */
        void try_pdu_mode_state_change()
        {
            for (int i = 0; i < this->pdu_meta_data_->asset_num; i++) {
                if (this->pdu_meta_data_->asset_pdu_check_status[i] == false) {
                    return;
                }
            }
            for (int i = 0; i < HAKO_PDU_CHANNEL_MAX; i++) {
                this->pdu_meta_data_->is_dirty[i] = false;
            }
            this->pdu_meta_data_->mode = HakoTimeMode_Asset;
        }
        /*
         * for master api
         */
        void create(uint32_t asset_num)
        {
            if (this->pdu_ != nullptr) {
                this->pdu_meta_data_->asset_num = asset_num;
                this->pdu_meta_data_->mode = HakoTimeMode_Asset;
                std::cout << "ALREADY CREATED PDU DATA" << std::endl;
                return;
            }
            this->pdu_meta_data_->mode = HakoTimeMode_Asset;

            ssize_t total_size = this->pdu_total_size();
            std::cout << "START CREATE PDU DATA: total_size= " << total_size << std::endl;
            auto shmid = this->asset_shmp_->create_memory(HAKO_SHARED_MEMORY_ID_1, total_size);
            HAKO_ASSERT(shmid >= 0);
            void *datap = this->asset_shmp_->lock_memory(HAKO_SHARED_MEMORY_ID_1);
            this->pdu_ = static_cast<char*>(datap);
            {
                int off = 0;
                this->pdu_meta_data_->asset_num = asset_num;
                for (int i = 0; i < HAKO_PDU_CHANNEL_MAX; i++) {
                    this->pdu_meta_data_->channel[i].offset = off;
                    off += this->pdu_meta_data_->channel[i].size;
                }
                memset(this->pdu_, 0, total_size);
            }
            this->asset_shmp_->unlock_memory(HAKO_SHARED_MEMORY_ID_1);
            std::cout << "PDU DATA CREATED" << std::endl;
            printf("CREATED ADDR=%p\n", datap);
        }
        /*
         * for master api
         */
        void reset()
        {
            std::cout << "EVENT: reset" << std::endl;
            (void)this->master_shmp_->lock_memory(HAKO_SHARED_MEMORY_ID_0);
            {
                //this->pdu_meta_data_->asset_num = 0;
                //this->pdu_meta_data_->channel_num = 0;
                this->pdu_meta_data_->pdu_sync_asset_id = 0;
                this->pdu_meta_data_->mode = HakoTimeMode_Asset;
                for (int i = 0; i < HAKO_DATA_MAX_ASSET_NUM; i++) {
                    this->pdu_meta_data_->asset_pdu_check_status[i] = false;
                }
                for (int i = 0; i < HAKO_PDU_CHANNEL_MAX; i++) {
                    //this->pdu_meta_data_->channel[i].offset = 0;
                    //this->pdu_meta_data_->channel[i].size = 0;
                    this->pdu_meta_data_->is_dirty[i] = false;
                    this->pdu_meta_data_->is_rbusy[i] = false;
                    this->pdu_meta_data_->is_wbusy[i] = false;
                }
                ssize_t total_size = this->pdu_total_size();
                memset(this->pdu_, 0, total_size);
            }
            this->master_shmp_->unlock_memory(HAKO_SHARED_MEMORY_ID_0);
        }
        bool is_pdu_created()
        {
            if ((this->pdu_ == nullptr) && (this->pdu_meta_data_->asset_num > 0)) {
                this->load();
            }
            if (this->pdu_meta_data_->asset_num == 0) {
                return false;
            }
            else {
                return true;
            }
        }
        bool load()
        {
            if (this->pdu_ != nullptr) {
                return true;
            }
            ssize_t total_size = this->pdu_total_size();
            void *datap = this->asset_shmp_->load_memory(HAKO_SHARED_MEMORY_ID_1, total_size);
            if (datap == nullptr) {
                return false;
            }
            std::cout << "LOADED: PDU DATA" << std::endl;
            this->pdu_ = static_cast<char*>(datap);
            return true;
        }
        ssize_t pdu_total_size()
        {
            ssize_t total_size = 0;
            for (int i = 0; i < HAKO_PDU_CHANNEL_MAX; i++)
            {
                total_size += this->pdu_meta_data_->channel[i].size;
            }
            return total_size;
        }
        /*
         * for master api
         */
        void destroy()
        {
            if (this->asset_shmp_ != nullptr) {
                this->asset_shmp_->destroy_memory(HAKO_SHARED_MEMORY_ID_1);
                this->asset_shmp_ = nullptr;
                this->pdu_meta_data_ = nullptr;
                this->pdu_ = nullptr;
            }
       }
    private:
        char*               pdu_;
        HakoPduMetaDataType *pdu_meta_data_ = nullptr;
        /**
         * TODO
         * なぜかmaster_shmp_にHAKO_SHARED_MEMORY_ID_1を追加しようとすると
         * load()で変なアドレスが帰ってくる。。おそらくHakoSharedMemoryが
         * 複数 key 対応できてないような気がする。
         * 一旦は、実装スピード重視で、HakoSharedMemoryは 1 key のみの制限をつける
         */
        std::shared_ptr<hako::utils::HakoSharedMemory>  asset_shmp_;
        std::shared_ptr<hako::utils::HakoSharedMemory>  master_shmp_;
    };
}

#endif /* _HAKO_PDU_DATA_HPP_ */
