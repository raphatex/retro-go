#include "rg_system.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(RG_STORAGE_SDSPI_HOST)
#include <driver/sdspi_host.h>
#define SDCARD_DO_TRANSACTION sdspi_host_do_transaction
#elif defined(RG_STORAGE_SDMMC_HOST)
#include <driver/sdmmc_host.h>
#define SDCARD_DO_TRANSACTION sdmmc_host_do_transaction
#endif

#ifdef ESP_PLATFORM
#include <esp_vfs_fat.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#include <windows.h>
#define access _access
#define mkdir(A, B) mkdir(A)
#else
#include <dirent.h>
#include <unistd.h>
#endif

static bool disk_mounted = false;
static bool disk_led = true;
#if defined(RG_STORAGE_SDSPI_HOST) || defined(RG_STORAGE_SDMMC_HOST)
static sdmmc_card_t *card_handle = NULL;
#endif
#if defined(RG_STORAGE_FLASH_PARTITION)
static wl_handle_t wl_handle = WL_INVALID_HANDLE;
#endif

static const char *SETTING_DISK_ACTIVITY = "DiskActivity";

#define CHECK_PATH(path)          \
    if (!(path && path[0]))       \
    {                             \
        RG_LOGE("No path given"); \
        return false;             \
    }

void rg_storage_set_activity_led(bool enable)
{
    rg_settings_set_number(NS_GLOBAL, SETTING_DISK_ACTIVITY, enable);
    disk_led = enable;
}

bool rg_storage_get_activity_led(void)
{
    return rg_settings_get_number(NS_GLOBAL, SETTING_DISK_ACTIVITY, disk_led);
}

#if defined(RG_STORAGE_SDSPI_HOST) || defined(RG_STORAGE_SDMMC_HOST)
static esp_err_t sdcard_do_transaction(int slot, sdmmc_command_t *cmdinfo)
{
    bool use_led = (disk_led && !rg_system_get_led());

    if (use_led)
        rg_system_set_led(1);

    esp_err_t ret = SDCARD_DO_TRANSACTION(slot, cmdinfo);
    if (ret == ESP_ERR_NO_MEM)
    {
        // free some memory and try again?
    }

    if (use_led)
        rg_system_set_led(0);

    return ret;
}
#endif

void rg_storage_init(void)
{
    RG_ASSERT(!disk_mounted, "Storage already initialized!");
    int error_code = -1;

#if defined(RG_STORAGE_SDSPI_HOST)

    RG_LOGI("Looking for SD Card using SDSPI...");

    sdmmc_host_t host_config = SDSPI_HOST_DEFAULT();
    host_config.slot = RG_STORAGE_SDSPI_HOST;
    host_config.max_freq_khz = RG_STORAGE_SDSPI_SPEED;
    host_config.do_transaction = &sdcard_do_transaction;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.host_id = RG_STORAGE_SDSPI_HOST;
    slot_config.gpio_cs = RG_GPIO_SDSPI_CS;

    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 4,
        .allocation_unit_size = 0,
    };

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = RG_GPIO_SDSPI_MOSI,
        .miso_io_num = RG_GPIO_SDSPI_MISO,
        .sclk_io_num = RG_GPIO_SDSPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    esp_err_t err = spi_bus_initialize(RG_STORAGE_SDSPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) // check but do not abort, let esp_vfs_fat_sdspi_mount decide
        RG_LOGW("SPI bus init failed (0x%x)", err);

    err = esp_vfs_fat_sdspi_mount(RG_STORAGE_ROOT, &host_config, &slot_config, &mount_config, &card_handle);
    if (err == ESP_ERR_TIMEOUT || err == ESP_ERR_INVALID_RESPONSE || err == ESP_ERR_INVALID_CRC)
    {
        RG_LOGW("SD Card mounting failed (0x%x), retrying at lower speed...\n", err);
        host_config.max_freq_khz = SDMMC_FREQ_PROBING;
        err = esp_vfs_fat_sdspi_mount(RG_STORAGE_ROOT, &host_config, &slot_config, &mount_config, &card_handle);
    }
    error_code = (int)err;

#elif defined(RG_STORAGE_SDMMC_HOST)

    RG_LOGI("Looking for SD Card using SDMMC...");

    sdmmc_host_t host_config = SDMMC_HOST_DEFAULT();
    host_config.flags = SDMMC_HOST_FLAG_1BIT;
    host_config.slot = RG_STORAGE_SDMMC_HOST;
    host_config.max_freq_khz = RG_STORAGE_SDMMC_SPEED;
    host_config.do_transaction = &sdcard_do_transaction;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
#if SOC_SDMMC_USE_GPIO_MATRIX
    slot_config.clk = RG_GPIO_SDSPI_CLK;
    slot_config.cmd = RG_GPIO_SDSPI_CMD;
    slot_config.d0 = RG_GPIO_SDSPI_D0;
    // d1 and d3 normally not used in width=1 but sdmmc_host_init_slot saves them, so just in case
    slot_config.d1 = slot_config.d3 = -1;
#endif

    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 4,
        .allocation_unit_size = 0,
    };

    esp_err_t err = esp_vfs_fat_sdmmc_mount(RG_STORAGE_ROOT, &host_config, &slot_config, &mount_config, &card_handle);
    if (err == ESP_ERR_TIMEOUT || err == ESP_ERR_INVALID_RESPONSE || err == ESP_ERR_INVALID_CRC)
    {
        RG_LOGW("SD Card mounting failed (0x%x), retrying at lower speed...\n", err);
        host_config.max_freq_khz = SDMMC_FREQ_PROBING;
        err = esp_vfs_fat_sdmmc_mount(RG_STORAGE_ROOT, &host_config, &slot_config, &mount_config, &card_handle);
    }
    error_code = (int)err;

#elif defined(RG_STORAGE_USBOTG_HOST)

    #warning "USB OTG isn't available on your SOC"
    RG_LOGI("Looking for USB mass storage...");
    error_code = -1;

#elif !defined(RG_STORAGE_FLASH_PARTITION)

    RG_LOGI("Using host (stdlib) for storage.");
    // Maybe we should just check if RG_STORAGE_ROOT exists?
    error_code = 0;

#endif

#if defined(RG_STORAGE_FLASH_PARTITION) // SPI Internal Flash

    if (error_code) // only if no previous storage was successfully mounted already
    {
        RG_LOGI("Looking for an internal flash partition labelled '%s' to mount for storage...", RG_STORAGE_FLASH_PARTITION);

        esp_vfs_fat_mount_config_t mount_config = {
            .format_if_mount_failed = true, // if mount failed, it's probably because it's a clean install so the partition hasn't been formatted yet
            .max_files = 4, // must be initialized, otherwise it will be 0, which doesn't make sense, and will trigger an ESP_ERR_NO_MEM error
        };

        esp_err_t err = esp_vfs_fat_spiflash_mount(RG_STORAGE_ROOT, RG_STORAGE_FLASH_PARTITION, &mount_config, &wl_handle);
        error_code = (int)err;
    }

#endif

    disk_mounted = !error_code;

    if (disk_mounted)
        RG_LOGI("Storage mounted at %s.", RG_STORAGE_ROOT);
    else
        RG_LOGE("Storage mounting failed! err=0x%x", error_code);
}

void rg_storage_deinit(void)
{
    if (!disk_mounted)
        return;

    rg_storage_commit();

    int error_code = 0;

#if defined(RG_STORAGE_SDSPI_HOST) || defined(RG_STORAGE_SDMMC_HOST)
    if (card_handle != NULL)
    {
        esp_err_t err = esp_vfs_fat_sdcard_unmount(RG_STORAGE_ROOT, card_handle);
        card_handle = NULL; // NULL it regardless of success, nothing we can do on errors...
        error_code = (int)err;
    }
#endif

#if defined(RG_STORAGE_FLASH_PARTITION)
    if (wl_handle != WL_INVALID_HANDLE)
    {
        esp_err_t err = esp_vfs_fat_spiflash_unmount(RG_STORAGE_ROOT, wl_handle);
        wl_handle = WL_INVALID_HANDLE;
        error_code = (int)err;
    }
#endif

    if (error_code)
        RG_LOGE("Storage unmounting failed. err=0x%x", error_code);
    else
        RG_LOGI("Storage unmounted.");

    disk_mounted = false;
}

bool rg_storage_ready(void)
{
    return disk_mounted;
}

void rg_storage_commit(void)
{
    if (!disk_mounted)
        return;
    // flush buffers();
}

bool rg_storage_mkdir(const char *dir)
{
    CHECK_PATH(dir);

    if (mkdir(dir, 0777) == 0)
        return true;

    // FIXME: Might want to stat to see if it's a dir
    if (errno == EEXIST)
        return true;

    // Possibly missing some parents, try creating them
    char *temp = strdup(dir);
    for (char *p = temp + strlen(RG_STORAGE_ROOT) + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
            if (strlen(temp) > 0)
            {
                mkdir(temp, 0777);
            }
            *p = '/';
            while (*(p + 1) == '/')
                p++;
        }
    }
    free(temp);

    // Finally try again
    if (mkdir(dir, 0777) == 0)
        return true;

    return false;
}

static int delete_cb(const rg_scandir_t *file, void *arg)
{
    rg_storage_delete(file->path);
    return RG_SCANDIR_CONTINUE;
}

bool rg_storage_delete(const char *path)
{
    CHECK_PATH(path);

    // Try the fast way first
    if (remove(path) == 0 || rmdir(path) == 0)
        return true;

    // If that fails, it's likely a non-empty directory and we go recursive
    // (errno could confirm but it has proven unreliable across platforms...)
    if (rg_storage_scandir(path, delete_cb, NULL, 0))
        return rmdir(path) == 0;

    return false;
}

rg_stat_t rg_storage_stat(const char *path)
{
    rg_stat_t ret = {0};
    struct stat statbuf;
    if (path && stat(path, &statbuf) == 0)
    {
        ret.basename = rg_basename(path);
        ret.extension = rg_extension(path);
        ret.size = statbuf.st_size;
        ret.mtime = statbuf.st_mtime;
        ret.is_file = S_ISREG(statbuf.st_mode);
        ret.is_dir = S_ISDIR(statbuf.st_mode);
        ret.exists = true;
    }
    return ret;
}

bool rg_storage_exists(const char *path)
{
    CHECK_PATH(path);
    return access(path, F_OK) == 0;
}

bool rg_storage_scandir(const char *path, rg_scandir_cb_t *callback, void *arg, uint32_t flags)
{
    CHECK_PATH(path);
    uint32_t types = flags & (RG_SCANDIR_FILES | RG_SCANDIR_DIRS);
    size_t path_len = strlen(path) + 1;
    struct stat statbuf;
    struct dirent *ent;

    if (path_len > RG_PATH_MAX - 5)
    {
        RG_LOGE("Folder path too long '%s'", path);
        return false;
    }

    DIR *dir = opendir(path);
    if (!dir)
        return false;

    // We allocate on heap in case we go recursive through rg_storage_delete
    rg_scandir_t *result = calloc(1, sizeof(rg_scandir_t));
    if (!result)
    {
        closedir(dir);
        return false;
    }

    strcat(strcpy(result->path, path), "/");
    result->basename = result->path + path_len;
    result->dirname = path;

    while ((ent = readdir(dir)))
    {
        if (ent->d_name[0] == '.' && (!ent->d_name[1] || ent->d_name[1] == '.'))
        {
            // Skip self and parent
            continue;
        }

        if (path_len + strlen(ent->d_name) >= RG_PATH_MAX)
        {
            RG_LOGE("File path too long '%s/%s'", path, ent->d_name);
            continue;
        }

        strcpy((char *)result->basename, ent->d_name);
    #if defined(DT_REG) && defined(DT_DIR)
        result->is_file = ent->d_type == DT_REG;
        result->is_dir = ent->d_type == DT_DIR;
    #else
        result->is_file = 0;
        result->is_dir = 0;
        // We're forced to stat() if the OS doesn't provide type via dirent
        flags |= RG_SCANDIR_STAT;
    #endif

        if ((flags & RG_SCANDIR_STAT) && stat(result->path, &statbuf) == 0)
        {
            result->is_file = S_ISREG(statbuf.st_mode);
            result->is_dir = S_ISDIR(statbuf.st_mode);
            result->size = statbuf.st_size;
            result->mtime = statbuf.st_mtime;
        }

        if ((result->is_dir && types != RG_SCANDIR_FILES) || (result->is_file && types != RG_SCANDIR_DIRS))
        {
            int ret = (callback)(result, arg);

            if (ret == RG_SCANDIR_STOP)
                break;

            if (ret == RG_SCANDIR_SKIP)
                continue;
        }

        if ((flags & RG_SCANDIR_RECURSIVE) && result->is_dir)
        {
            rg_storage_scandir(result->path, callback, arg, flags);
        }
    }

    closedir(dir);
    free(result);

    return true;
}

#if RG_HAVE_MINIZ
/**
 * This is a minimal UNZIP implementation that utilizes only the miniz primitives found in ESP32's ROM.
 * I think that we should use miniz' ZIP API instead and bundle miniz with retro-go. But first I need
 * to do some testing to determine if the increased executable size is acceptable...
 */
#include <rom/miniz.h>

#define ZIP_MAGIC 0x04034b50
typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint16_t compression;
    uint16_t modified_time;
    uint16_t modified_date;
    uint32_t checksum;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_size;
    uint16_t extra_field_size;
    uint8_t filename[226];
} zip_header_t;

bool rg_storage_unzip_file(const char *zip_path, const char *filter, void **data_out, size_t *data_len)
{
    RG_ASSERT(data_out && data_len, "Bad param");
    CHECK_PATH(zip_path);

    zip_header_t header = {0};
    size_t data_align = 0x2000;
    int header_pos = 0;

    FILE *fp = fopen(zip_path, "rb");
    if (!fp)
    {
        RG_LOGE("Fopen failed");
        return false;
    }

    // Very inefficient, we should read a block at a time and search it for a header. But I'm lazy.
    // Thankfully the header is usually found on the very first read :)
    for (header_pos = 0; !feof(fp); ++header_pos)
    {
        fseek(fp, header_pos, SEEK_SET);
        fread(&header, sizeof(header), 1, fp);
        if (header.magic == ZIP_MAGIC)
            break;
    }

    if (header.magic != ZIP_MAGIC)
    {
        RG_LOGE("No valid header found!");
        fclose(fp);
        return false;
    }

    // Zero terminate or truncate filename just in case
    header.filename[RG_MIN(header.filename_size, 225)] = 0;

    RG_LOGI("Found file at %d, name: '%s'", header_pos, header.filename);

    size_t stream_offset = header_pos + 30 + header.filename_size + header.extra_field_size;
    size_t uncompressed_size = header.uncompressed_size;
    void *uncompressed_stream = malloc(((uncompressed_size & ~data_align) + data_align));
    size_t compressed_size = header.compressed_size;
    void *compressed_stream = malloc(compressed_size);
    tinfl_decompressor *decomp = malloc(sizeof(tinfl_decompressor));
    tinfl_init(decomp);

    if (!compressed_stream || !uncompressed_stream || !decomp)
    {
        RG_LOGE("Out of memory");
        goto _fail;
    }

    // TODO: decompress in chunk to reduce memory usage
    if (fseek(fp, stream_offset, SEEK_SET) != 0 || fread(compressed_stream, compressed_size, 1, fp) != 1)
    {
        RG_LOGE("Read failure");
        goto _fail;
    }

    tinfl_status status =
        tinfl_decompress(decomp, compressed_stream, &compressed_size, uncompressed_stream, uncompressed_stream,
                         &uncompressed_size, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);

    if (status != TINFL_STATUS_DONE)
    {
        RG_LOGE("Decompression failed! ret: %d", (int)status);
        goto _fail;
    }

    free(compressed_stream);
    free(decomp);
    fclose(fp);

    *data_out = uncompressed_stream;
    *data_len = uncompressed_size;
    return true;

_fail:
    free(uncompressed_stream);
    free(compressed_stream);
    free(decomp);
    fclose(fp);
    return false;
}

#endif
