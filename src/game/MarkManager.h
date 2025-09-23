#pragma once
#include "MarkImage.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <future>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <queue>
#include "lzo_manager.h"

struct GuildMarkData {
    uint32_t guild_id;
    uint32_t mark_id;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t original_size;
    std::vector<uint8_t> data;
    std::string format;
    uint64_t timestamp;

    GuildMarkData() : guild_id(0), mark_id(0), crc32(0),
                     compressed_size(0), original_size(0), timestamp(0) {}
};

struct UploadRequest {
    uint32_t guild_id;
    std::vector<uint8_t> image_data;
    std::string format;
    std::promise<bool> result;

    UploadRequest(uint32_t gid, const std::vector<uint8_t>& data, const std::string& fmt)
        : guild_id(gid), image_data(data), format(fmt) {}
};

struct DownloadRequest {
    uint32_t guild_id;
    std::promise<std::shared_ptr<GuildMarkData>> result;

    explicit DownloadRequest(uint32_t gid) : guild_id(gid) {}
};

class CGuildMarkManager : public singleton<CGuildMarkManager>
{
	public:
		enum
		{
			MAX_IMAGE_COUNT = 5,
			INVALID_MARK_ID = 0xffffffff,
		};

		// Symbol
		struct TGuildSymbol
		{
			DWORD crc;
			std::vector<BYTE> raw;
		};

		CGuildMarkManager();
		virtual ~CGuildMarkManager();

		const TGuildSymbol * GetGuildSymbol(DWORD GID);
		bool LoadSymbol(const char* filename);
		void SaveSymbol(const char* filename);
		void UploadSymbol(DWORD guildID, int iSize, const BYTE* pbyData);

		//
		// Mark
		//
		void SetMarkPathPrefix(const char * prefix);

		bool LoadMarkIndex(); // 마크 인덱스 불러오기 (서버에서만 사용)
		bool SaveMarkIndex(); // 마크 인덱스 저장하기

		void LoadMarkImages(); // 모든 마크 이미지를 불러오기
		void SaveMarkImage(DWORD imgIdx); // 마크 이미지 저장

		bool GetMarkImageFilename(DWORD imgIdx, std::string & path) const;
		bool AddMarkIDByGuildID(DWORD guildID, DWORD markID);
		DWORD GetMarkImageCount() const;
		DWORD GetMarkCount() const;
		DWORD GetMarkID(DWORD guildID);

		// SERVER
		void CopyMarkIdx(char * pcBuf) const;
		DWORD SaveMark(DWORD guildID, BYTE * pbMarkImage);
		void DeleteMark(DWORD guildID);
		void GetDiffBlocks(DWORD imgIdx, const uint32_t* crcList, std::map<BYTE, const SGuildMarkBlock *> & mapDiffBlocks);

		// CLIENT
		bool SaveBlockFromCompressedData(DWORD imgIdx, DWORD idBlock, const BYTE * pbBlock, DWORD dwSize);
		bool GetBlockCRCList(DWORD imgIdx, DWORD * crcList);

		// NEW GUILD MARK SYSTEM
		bool InitializeNewSystem(const std::string& directory);
		void ShutdownNewSystem();
		std::future<bool> UploadAsync(uint32_t guild_id, const std::vector<uint8_t>& data, const std::string& format);
		void DeleteMarkAsync(uint32_t guild_id);
		std::future<std::shared_ptr<GuildMarkData>> DownloadAsync(uint32_t guild_id);
		bool GetMarkSync(uint32_t guild_id, GuildMarkData& data);
		bool HasMark(uint32_t guild_id) const;
		void DeleteMark(uint32_t guild_id);
		std::vector<uint32_t> GetAllGuilds() const;
		uint32_t GetNewMarkCount() const;

	private:
		// 
		// Mark
		//
		CGuildMarkImage * __NewImage();
		void __DeleteImage(CGuildMarkImage * pkImgDel);

		DWORD __AllocMarkID(DWORD guildID);

		CGuildMarkImage * __GetImage(DWORD imgIdx);
		CGuildMarkImage * __GetImagePtr(DWORD idMark);

		std::map<DWORD, CGuildMarkImage *> m_mapIdx_Image; // index = image index
		std::map<DWORD, DWORD> m_mapGID_MarkID; // index = guild id

		std::set<DWORD> m_setFreeMarkID;
		std::string		m_pathPrefix;

	private:
		//
		// Symbol
		//
		std::map<DWORD, TGuildSymbol> m_mapSymbol;

		// NEW GUILD MARK SYSTEM
		std::string m_mark_directory;
		std::string m_index_file;
		std::unordered_map<uint32_t, std::shared_ptr<GuildMarkData>> m_new_marks;
		mutable std::mutex m_new_marks_mutex;
		std::queue<std::unique_ptr<UploadRequest>> m_upload_queue;
		std::queue<std::unique_ptr<DownloadRequest>> m_download_queue;
		std::mutex m_upload_mutex;
		std::mutex m_download_mutex;
		std::condition_variable m_upload_cv;
		std::condition_variable m_download_cv;
		std::vector<std::thread> m_workers;
		std::atomic<bool> m_shutdown;
		std::atomic<uint32_t> m_next_id;

		void WorkerThread();
		void ProcessUpload(std::unique_ptr<UploadRequest> request);
		void ProcessDownload(std::unique_ptr<DownloadRequest> request);
		bool ValidateFormat(const std::vector<uint8_t>& data, const std::string& format);
		bool ValidateSize(const std::vector<uint8_t>& data, const std::string& format);
		std::vector<uint8_t> Compress(const std::vector<uint8_t>& data);
		std::vector<uint8_t> Decompress(const std::vector<uint8_t>& data, uint32_t size);
		uint32_t CalcCRC32(const std::vector<uint8_t>& data);
		uint64_t GetTimestamp();
		std::string GetFilePath(uint32_t mark_id) const;
		bool SaveToFile(const GuildMarkData& data);
		bool LoadFromFile(uint32_t mark_id, GuildMarkData& data);
		uint32_t AllocateId();
		bool SaveIndex();
		bool LoadIndex();
};
