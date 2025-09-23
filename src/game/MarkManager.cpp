#include "stdafx.h"
#include "MarkManager.h"

#include "crc32.h"

#include <iterator>
#include <fstream>
#include <filesystem>
#include <chrono>
#include "lzo_manager.h"

CGuildMarkImage * CGuildMarkManager::__NewImage()
{
	return M2_NEW CGuildMarkImage;
}

void CGuildMarkManager::__DeleteImage(CGuildMarkImage * pkImgDel)
{
	M2_DELETE(pkImgDel);
}

CGuildMarkManager::CGuildMarkManager()
{
	// 남은 mark id 셋을 만든다. (서버용)
	for (DWORD i = 0; i < MAX_IMAGE_COUNT * CGuildMarkImage::MARK_TOTAL_COUNT; ++i)
		m_setFreeMarkID.insert(i);
}

CGuildMarkManager::~CGuildMarkManager()
{
	for (std::map<DWORD, CGuildMarkImage *>::iterator it = m_mapIdx_Image.begin(); it != m_mapIdx_Image.end(); ++it)
		__DeleteImage(it->second);

	m_mapIdx_Image.clear();
}

bool CGuildMarkManager::GetMarkImageFilename(DWORD imgIdx, std::string & path) const
{
	if (imgIdx >= MAX_IMAGE_COUNT)
		return false;

	char buf[64];
	snprintf(buf, sizeof(buf), "mark/%s_%u.tga", m_pathPrefix.c_str(), imgIdx);
	path = buf;
	return true;
}

void CGuildMarkManager::SetMarkPathPrefix(const char * prefix)
{
	m_pathPrefix = prefix;
}

// 마크 인덱스 불러오기 (서버에서만 사용)
bool CGuildMarkManager::LoadMarkIndex()
{
	char buf[64];
	snprintf(buf, sizeof(buf), "mark/%s_index", m_pathPrefix.c_str());
	FILE * fp = fopen(buf, "r");

	if (!fp)
		return false;

	DWORD guildID;
	DWORD markID;

	char line[256];

	while (fgets(line, sizeof(line)-1, fp))
	{
		sscanf(line, "%u %u", &guildID, &markID);
		line[0] = '\0';
		AddMarkIDByGuildID(guildID, markID);
	}

	LoadMarkImages();

	fclose(fp);
	return true;
}

bool CGuildMarkManager::SaveMarkIndex()
{
	char buf[64];
	snprintf(buf, sizeof(buf), "mark/%s_index", m_pathPrefix.c_str());
	FILE * fp = fopen(buf, "w");

	if (!fp)
	{
		sys_err("MarkManager::SaveMarkIndex: cannot open index file.");
		return false;
	}

	for (std::map<DWORD, DWORD>::iterator it = m_mapGID_MarkID.begin(); it != m_mapGID_MarkID.end(); ++it)
		fprintf(fp, "%u %u\n", it->first, it->second);

	fclose(fp);
	sys_log(0, "MarkManager::SaveMarkIndex: index count %d", m_mapGID_MarkID.size());
	return true;
}

void CGuildMarkManager::LoadMarkImages()
{
	bool isMarkExists[MAX_IMAGE_COUNT];
	memset(isMarkExists, 0, sizeof(isMarkExists));

	for (std::map<DWORD, DWORD>::iterator it = m_mapGID_MarkID.begin(); it != m_mapGID_MarkID.end(); ++it)
	{
		DWORD markID = it->second;

		if (markID < MAX_IMAGE_COUNT * CGuildMarkImage::MARK_TOTAL_COUNT)
			isMarkExists[markID / CGuildMarkImage::MARK_TOTAL_COUNT] = true;
	}

	for (DWORD i = 0; i < MAX_IMAGE_COUNT; ++i)
		if (isMarkExists[i])
			__GetImage(i);
}

void CGuildMarkManager::SaveMarkImage(DWORD imgIdx)
{
	std::string path;

	if (GetMarkImageFilename(imgIdx, path))
		if (!__GetImage(imgIdx)->Save(path.c_str()))
			sys_err("%s Save failed\n", path.c_str());
}

CGuildMarkImage * CGuildMarkManager::__GetImage(DWORD imgIdx)
{
	std::map<DWORD, CGuildMarkImage *>::iterator it = m_mapIdx_Image.find(imgIdx);

	if (it == m_mapIdx_Image.end())
	{
		std::string imagePath;

		if (GetMarkImageFilename(imgIdx, imagePath))
		{
			CGuildMarkImage * pkImage = __NewImage();
			m_mapIdx_Image.insert(std::map<DWORD, CGuildMarkImage *>::value_type(imgIdx, pkImage));
			
			if (!pkImage->Load(imagePath.c_str()))
			{
				pkImage->Build(imagePath.c_str());
				pkImage->Load(imagePath.c_str());
			}

			return pkImage;
		}
		else
			return NULL;
	}
	else
		return it->second;
}

bool CGuildMarkManager::AddMarkIDByGuildID(DWORD guildID, DWORD markID)
{
	if (markID >= MAX_IMAGE_COUNT * CGuildMarkImage::MARK_TOTAL_COUNT)
		return false;

	//sys_log(0, "MarkManager: guild_id=%d mark_id=%d", guildID, markID);
	m_mapGID_MarkID.insert(std::map<DWORD, DWORD>::value_type(guildID, markID));
	m_setFreeMarkID.erase(markID);
	return true;
}

DWORD CGuildMarkManager::GetMarkID(DWORD guildID)
{
	std::map<DWORD, DWORD>::iterator it = m_mapGID_MarkID.find(guildID);

	if (it == m_mapGID_MarkID.end())
		return INVALID_MARK_ID;

	return it->second;
}

DWORD CGuildMarkManager::__AllocMarkID(DWORD guildID)
{
	std::set<DWORD>::iterator it = m_setFreeMarkID.lower_bound(0);

	if (it == m_setFreeMarkID.end())
		return INVALID_MARK_ID;

	DWORD markID = *it;
	
	DWORD imgIdx = markID / CGuildMarkImage::MARK_TOTAL_COUNT;
	CGuildMarkImage * pkImage = __GetImage(imgIdx); // 이미지가 없다면 만들기 위해 

	if (pkImage && AddMarkIDByGuildID(guildID, markID))
		return markID;

	return INVALID_MARK_ID;
}

DWORD CGuildMarkManager::GetMarkImageCount() const
{
	return m_mapIdx_Image.size();
}

DWORD CGuildMarkManager::GetMarkCount() const
{
	return m_mapGID_MarkID.size();
}

// SERVER
void CGuildMarkManager::CopyMarkIdx(char * pcBuf) const
{
	WORD * pwBuf = (WORD *) pcBuf;

	for (std::map<DWORD, DWORD>::const_iterator it = m_mapGID_MarkID.begin(); it != m_mapGID_MarkID.end(); ++it)
	{
		*(pwBuf++) = it->first; // guild id
		*(pwBuf++) = it->second; // mark id
	}
}

// SERVER
DWORD CGuildMarkManager::SaveMark(DWORD guildID, BYTE * pbMarkImage)
{
	DWORD idMark;

	if ((idMark = GetMarkID(guildID)) == INVALID_MARK_ID)
	{
		if ((idMark = __AllocMarkID(guildID)) == INVALID_MARK_ID)
		{
			sys_err("CGuildMarkManager: cannot alloc mark id %u", guildID);
			return false;
		}
		else
			sys_log(0, "SaveMark: mark id alloc %u", idMark);
	}
	else
		sys_log(0, "SaveMark: mark id found %u", idMark);

	DWORD imgIdx = (idMark / CGuildMarkImage::MARK_TOTAL_COUNT);
	CGuildMarkImage * pkImage = __GetImage(imgIdx);

	if (pkImage)
	{
		pkImage->SaveMark(idMark % CGuildMarkImage::MARK_TOTAL_COUNT, pbMarkImage);

		SaveMarkImage(imgIdx);
		SaveMarkIndex();
	}

	return idMark;
}

// SERVER
void CGuildMarkManager::DeleteMark(DWORD guildID)
{
	std::map<DWORD, DWORD>::iterator it = m_mapGID_MarkID.find(guildID);

	if (it == m_mapGID_MarkID.end())
		return;

	CGuildMarkImage * pkImage;

	if ((pkImage = __GetImage(it->second / CGuildMarkImage::MARK_TOTAL_COUNT)) != NULL)
		pkImage->DeleteMark(it->second % CGuildMarkImage::MARK_TOTAL_COUNT);

	m_setFreeMarkID.insert(it->second);
	m_mapGID_MarkID.erase(it);

	SaveMarkIndex();
}

// SERVER
void CGuildMarkManager::GetDiffBlocks(DWORD imgIdx, const uint32_t* crcList, std::map<BYTE, const SGuildMarkBlock *> & mapDiffBlocks)
{
	mapDiffBlocks.clear();

	// 클라이언트에서 서버에 없는 이미지를 요청할 수는 없다.
	if (m_mapIdx_Image.end() == m_mapIdx_Image.find(imgIdx))
	{
		sys_err("invalid idx %u", imgIdx);
		return;
	}

	CGuildMarkImage * p = __GetImage(imgIdx);

	if (p)
		p->GetDiffBlocks(crcList, mapDiffBlocks);
}

// CLIENT
bool CGuildMarkManager::SaveBlockFromCompressedData(DWORD imgIdx, DWORD posBlock, const BYTE * pbBlock, DWORD dwSize)
{
	CGuildMarkImage * pkImage = __GetImage(imgIdx);

	if (pkImage)
		pkImage->SaveBlockFromCompressedData(posBlock, pbBlock, dwSize);

	return false;
}

// CLIENT
bool CGuildMarkManager::GetBlockCRCList(DWORD imgIdx, DWORD * crcList)
{
	// 클라이언트에서 서버에 없는 이미지를 요청할 수는 없다.
	if (m_mapIdx_Image.end() == m_mapIdx_Image.find(imgIdx))
	{
		sys_err("invalid idx %u", imgIdx);
		return false;
	}

	CGuildMarkImage * p = __GetImage(imgIdx);
	
	if (p)
		p->GetBlockCRCList(crcList);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////
// Symbol
///////////////////////////////////////////////////////////////////////////////////////
const CGuildMarkManager::TGuildSymbol * CGuildMarkManager::GetGuildSymbol(DWORD guildID)
{
	std::map<DWORD, TGuildSymbol>::iterator it = m_mapSymbol.find(guildID);

	if (it == m_mapSymbol.end())
		return NULL;

	return &it->second;
}

bool CGuildMarkManager::LoadSymbol(const char* filename)
{
	FILE* fp = fopen(filename, "rb");

	if (!fp)
		return true;
	else
	{
		DWORD symbolCount;
		fread(&symbolCount, 4, 1, fp);

		for (DWORD i = 0; i < symbolCount; i++)
		{
			DWORD guildID;
			DWORD dwSize;
			fread(&guildID, 4, 1, fp);
			fread(&dwSize, 4, 1, fp);

			TGuildSymbol gs;
			gs.raw.resize(dwSize);
			fread(&gs.raw[0], 1, dwSize, fp);
			gs.crc = GetCRC32(reinterpret_cast<const char*>(&gs.raw[0]), dwSize);
			m_mapSymbol.insert(std::make_pair(guildID, gs));
		}
	}

	fclose(fp);
	return true;
}

void CGuildMarkManager::SaveSymbol(const char* filename)
{
	FILE* fp = fopen(filename, "wb");
	if (!fp)
	{
		sys_err("Cannot open Symbol file (name: %s)", filename);
		return;
	}

	DWORD symbolCount = m_mapSymbol.size();
	fwrite(&symbolCount, 4, 1, fp);

	for (std::map<DWORD, TGuildSymbol>::iterator it = m_mapSymbol.begin(); it != m_mapSymbol.end(); ++it)
	{
		DWORD guildID = it->first;
		DWORD dwSize = it->second.raw.size();
		fwrite(&guildID, 4, 1, fp);
		fwrite(&dwSize, 4, 1, fp);
		fwrite(&it->second.raw[0], 1, dwSize, fp);
	}

	fclose(fp);
}

void CGuildMarkManager::UploadSymbol(DWORD guildID, int iSize, const BYTE* pbyData)
{
	sys_log(0, "GuildSymbolUpload guildID %u Size %d", guildID, iSize);
	
	if (m_mapSymbol.find(guildID) == m_mapSymbol.end())
		m_mapSymbol.insert(std::make_pair(guildID, TGuildSymbol()));

	TGuildSymbol& rSymbol = m_mapSymbol[guildID];
	rSymbol.raw.clear();

	if (iSize > 0)
	{
		rSymbol.raw.reserve(iSize);
		std::copy(pbyData, (pbyData + iSize), std::back_inserter(rSymbol.raw));
		rSymbol.crc = GetCRC32(reinterpret_cast<const char*>(pbyData), iSize);
	}
}

bool CGuildMarkManager::InitializeNewSystem(const std::string& directory) {
	m_mark_directory = directory;
	m_index_file = directory + "/index.dat";
	m_shutdown = false;
	m_next_id = 1;

	if (!std::filesystem::exists(m_mark_directory)) {
		std::filesystem::create_directories(m_mark_directory);
	}

	LoadIndex();

	for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
		m_workers.emplace_back([this] { WorkerThread(); });
	}

	return true;
}

void CGuildMarkManager::ShutdownNewSystem() {
	m_shutdown = true;
	m_upload_cv.notify_all();
	m_download_cv.notify_all();

	for (auto& worker : m_workers) {
		if (worker.joinable()) {
			worker.join();
		}
	}

	SaveIndex();
}

std::future<bool> CGuildMarkManager::UploadAsync(uint32_t guild_id, const std::vector<uint8_t>& data, const std::string& format) {
	auto request = std::make_unique<UploadRequest>(guild_id, data, format);
	auto future = request->result.get_future();

	{
		std::lock_guard<std::mutex> lock(m_upload_mutex);
		m_upload_queue.push(std::move(request));
	}
	m_upload_cv.notify_one();

	return future;
}

std::future<std::shared_ptr<GuildMarkData>> CGuildMarkManager::DownloadAsync(uint32_t guild_id) {
	auto request = std::make_unique<DownloadRequest>(guild_id);
	auto future = request->result.get_future();

	{
		std::lock_guard<std::mutex> lock(m_download_mutex);
		m_download_queue.push(std::move(request));
	}
	m_download_cv.notify_one();

	return future;
}

bool CGuildMarkManager::GetMarkSync(uint32_t guild_id, GuildMarkData& data) {
	std::lock_guard<std::mutex> lock(m_new_marks_mutex);
	auto it = m_new_marks.find(guild_id);
	if (it != m_new_marks.end()) {
		data = *it->second;
		return true;
	}
	return false;
}

bool CGuildMarkManager::HasMark(uint32_t guild_id) const {
	std::lock_guard<std::mutex> lock(m_new_marks_mutex);
	return m_new_marks.find(guild_id) != m_new_marks.end();
}

void CGuildMarkManager::DeleteMark(uint32_t guild_id) {
	std::lock_guard<std::mutex> lock(m_new_marks_mutex);
	auto it = m_new_marks.find(guild_id);
	if (it != m_new_marks.end()) {
		uint32_t mark_id = it->second->mark_id;
		m_new_marks.erase(it);

		std::string file_path = GetFilePath(mark_id);
		if (std::filesystem::exists(file_path)) {
			std::filesystem::remove(file_path);
		}
	}
}

void CGuildMarkManager::DeleteMarkAsync(uint32_t guild_id) {
	// For async deletion, just call the sync version since file deletion is fast
	DeleteMark(guild_id);
}

std::vector<uint32_t> CGuildMarkManager::GetAllGuilds() const {
	std::lock_guard<std::mutex> lock(m_new_marks_mutex);
	std::vector<uint32_t> guilds;
	guilds.reserve(m_new_marks.size());

	for (const auto& pair : m_new_marks) {
		guilds.push_back(pair.first);
	}

	return guilds;
}

uint32_t CGuildMarkManager::GetNewMarkCount() const {
	std::lock_guard<std::mutex> lock(m_new_marks_mutex);
	return m_new_marks.size();
}

void CGuildMarkManager::WorkerThread() {
	while (!m_shutdown) {
		std::unique_ptr<UploadRequest> upload_req;
		std::unique_ptr<DownloadRequest> download_req;

		{
			std::unique_lock<std::mutex> lock(m_upload_mutex);
			m_upload_cv.wait(lock, [this] { return !m_upload_queue.empty() || m_shutdown; });

			if (!m_upload_queue.empty()) {
				upload_req = std::move(m_upload_queue.front());
				m_upload_queue.pop();
			}
		}

		if (upload_req) {
			ProcessUpload(std::move(upload_req));
			continue;
		}

		{
			std::unique_lock<std::mutex> lock(m_download_mutex);
			m_download_cv.wait(lock, [this] { return !m_download_queue.empty() || m_shutdown; });

			if (!m_download_queue.empty()) {
				download_req = std::move(m_download_queue.front());
				m_download_queue.pop();
			}
		}

		if (download_req) {
			ProcessDownload(std::move(download_req));
		}
	}
}

void CGuildMarkManager::ProcessUpload(std::unique_ptr<UploadRequest> request) {
	bool success = false;

	if (ValidateFormat(request->image_data, request->format) &&
		ValidateSize(request->image_data, request->format)) {

		auto mark_data = std::make_shared<GuildMarkData>();
		mark_data->guild_id = request->guild_id;
		mark_data->mark_id = AllocateId();
		mark_data->format = request->format;
		mark_data->timestamp = GetTimestamp();
		mark_data->original_size = request->image_data.size();

		mark_data->data = Compress(request->image_data);
		mark_data->compressed_size = mark_data->data.size();
		mark_data->crc32 = CalcCRC32(request->image_data);

		if (SaveToFile(*mark_data)) {
			std::lock_guard<std::mutex> lock(m_new_marks_mutex);
			m_new_marks[request->guild_id] = mark_data;
			success = true;
		}
	}

	request->result.set_value(success);
}

void CGuildMarkManager::ProcessDownload(std::unique_ptr<DownloadRequest> request) {
	std::shared_ptr<GuildMarkData> result;

	{
		std::lock_guard<std::mutex> lock(m_new_marks_mutex);
		auto it = m_new_marks.find(request->guild_id);
		if (it != m_new_marks.end()) {
			result = it->second;
		}
	}

	request->result.set_value(result);
}

bool CGuildMarkManager::ValidateFormat(const std::vector<uint8_t>& data, const std::string& format) {
	if (data.empty()) return false;

	if (format == "png") {
		return data.size() >= 8 &&
			   data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E && data[3] == 0x47;
	}
	else if (format == "jpg" || format == "jpeg") {
		return data.size() >= 2 && data[0] == 0xFF && data[1] == 0xD8;
	}
	else if (format == "gif") {
		return data.size() >= 6 &&
			   data[0] == 0x47 && data[1] == 0x49 && data[2] == 0x46;
	}
	else if (format == "bmp") {
		return data.size() >= 2 && data[0] == 0x42 && data[1] == 0x4D;
	}

	return false;
}

bool CGuildMarkManager::ValidateSize(const std::vector<uint8_t>& data, const std::string& format) {
	return data.size() <= 1024 * 1024;
}

std::vector<uint8_t> CGuildMarkManager::Compress(const std::vector<uint8_t>& data) {
	// Check if data is already compressed format (PNG/JPEG have built-in compression)
	if (data.size() >= 8) {
		// PNG signature
		const uint8_t png_sig[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
		if (memcmp(data.data(), png_sig, 8) == 0) {
			return data; // PNG already compressed
		}

		// JPEG signature
		if (data[0] == 0xFF && data[1] == 0xD8) {
			return data; // JPEG already compressed
		}

		// GIF signatures
		if (memcmp(data.data(), "GIF87a", 6) == 0 || memcmp(data.data(), "GIF89a", 6) == 0) {
			return data; // GIF already compressed
		}
	}

	// Only compress raw/uncompressed formats like BMP or TGA
	lzo_uint max_size = LZOManager::Instance().GetMaxCompressedSize(data.size());
	std::vector<uint8_t> compressed(max_size);
	lzo_uint compressed_size = max_size;

	if (LZOManager::Instance().Compress(data.data(), data.size(), compressed.data(), &compressed_size)) {
		// Only use compression if it actually reduces size
		if (compressed_size < data.size()) {
			compressed.resize(compressed_size);
			return compressed;
		}
	}

	return data; // Return uncompressed if compression doesn't help
}

std::vector<uint8_t> CGuildMarkManager::Decompress(const std::vector<uint8_t>& data, uint32_t size) {
	// Use LZO decompression like original guild mark system
	std::vector<uint8_t> decompressed(size);
	lzo_uint decompressed_size = size;

	if (LZOManager::Instance().Decompress(data.data(), data.size(), decompressed.data(), &decompressed_size)) {
		return decompressed;
	}

	return data; // Return original data if decompression fails
}

uint32_t CGuildMarkManager::CalcCRC32(const std::vector<uint8_t>& data) {
	return GetCRC32(reinterpret_cast<const char*>(data.data()), data.size());
}

uint64_t CGuildMarkManager::GetTimestamp() {
	auto now = std::chrono::system_clock::now();
	return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

std::string CGuildMarkManager::GetFilePath(uint32_t mark_id) const {
	return m_mark_directory + "/mark_" + std::to_string(mark_id) + ".dat";
}

uint32_t CGuildMarkManager::AllocateId() {
	return m_next_id++;
}

bool CGuildMarkManager::SaveToFile(const GuildMarkData& data) {
	std::ofstream file(GetFilePath(data.mark_id), std::ios::binary);
	if (!file) return false;

	file.write(reinterpret_cast<const char*>(&data.guild_id), sizeof(data.guild_id));
	file.write(reinterpret_cast<const char*>(&data.mark_id), sizeof(data.mark_id));
	file.write(reinterpret_cast<const char*>(&data.crc32), sizeof(data.crc32));
	file.write(reinterpret_cast<const char*>(&data.compressed_size), sizeof(data.compressed_size));
	file.write(reinterpret_cast<const char*>(&data.original_size), sizeof(data.original_size));
	file.write(reinterpret_cast<const char*>(&data.timestamp), sizeof(data.timestamp));

	uint32_t format_len = data.format.length();
	file.write(reinterpret_cast<const char*>(&format_len), sizeof(format_len));
	file.write(data.format.c_str(), format_len);

	file.write(reinterpret_cast<const char*>(data.data.data()), data.data.size());

	return file.good();
}

bool CGuildMarkManager::LoadFromFile(uint32_t mark_id, GuildMarkData& data) {
	std::ifstream file(GetFilePath(mark_id), std::ios::binary);
	if (!file) return false;

	file.read(reinterpret_cast<char*>(&data.guild_id), sizeof(data.guild_id));
	file.read(reinterpret_cast<char*>(&data.mark_id), sizeof(data.mark_id));
	file.read(reinterpret_cast<char*>(&data.crc32), sizeof(data.crc32));
	file.read(reinterpret_cast<char*>(&data.compressed_size), sizeof(data.compressed_size));
	file.read(reinterpret_cast<char*>(&data.original_size), sizeof(data.original_size));
	file.read(reinterpret_cast<char*>(&data.timestamp), sizeof(data.timestamp));

	uint32_t format_len;
	file.read(reinterpret_cast<char*>(&format_len), sizeof(format_len));
	data.format.resize(format_len);
	file.read(&data.format[0], format_len);

	data.data.resize(data.compressed_size);
	file.read(reinterpret_cast<char*>(data.data.data()), data.compressed_size);

	return file.good();
}

bool CGuildMarkManager::SaveIndex() {
	std::ofstream file(m_index_file, std::ios::binary);
	if (!file) return false;

	std::lock_guard<std::mutex> lock(m_new_marks_mutex);
	uint32_t count = m_new_marks.size();
	file.write(reinterpret_cast<const char*>(&count), sizeof(count));
	file.write(reinterpret_cast<const char*>(&m_next_id), sizeof(m_next_id));

	for (const auto& pair : m_new_marks) {
		file.write(reinterpret_cast<const char*>(&pair.first), sizeof(pair.first));
		file.write(reinterpret_cast<const char*>(&pair.second->mark_id), sizeof(pair.second->mark_id));
	}

	return file.good();
}

bool CGuildMarkManager::LoadIndex() {
	std::ifstream file(m_index_file, std::ios::binary);
	if (!file) return false;

	uint32_t count;
	file.read(reinterpret_cast<char*>(&count), sizeof(count));
	file.read(reinterpret_cast<char*>(&m_next_id), sizeof(m_next_id));

	for (uint32_t i = 0; i < count; ++i) {
		uint32_t guild_id, mark_id;
		file.read(reinterpret_cast<char*>(&guild_id), sizeof(guild_id));
		file.read(reinterpret_cast<char*>(&mark_id), sizeof(mark_id));

		auto mark_data = std::make_shared<GuildMarkData>();
		if (LoadFromFile(mark_id, *mark_data)) {
			m_new_marks[guild_id] = mark_data;
		}
	}

	return true;
}
