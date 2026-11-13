#include "graphics/map/TileService.h"

void TileService::setService(ITileService *s)
{
    delete service;
    service = s;
}

void TileService::setBackupService(ITileService *s)
{
    delete backup;
    backup = s;
}

bool TileService::isAsync() const
{
    return (service && service->isAsync()) || (backup && backup->isAsync());
}

bool TileService::hasAsync() const
{
    return (service && service->hasAsync()) || (backup && backup->hasAsync());
}

bool TileService::loadSyncOnly(const char *name, void *img)
{
    if (service && !service->isAsync()) {
        if (service->load(name, img))
            return true;
    }
    if (backup && !backup->isAsync()) {
        if (backup->load(name, img))
            return true;
    }
    return false;
}

lv_image_dsc_t *TileService::loadRaw(const char *name)
{
    // This method is called only by the background tile loader. Synchronous
    // local services may expose loadRaw() too, keeping storage and decoding
    // off the LVGL task alongside remote services.
    if (service) {
        lv_image_dsc_t *result = service->loadRaw(name);
        if (result)
            return result;
    }
    if (backup) {
        return backup->loadRaw(name);
    }
    return nullptr;
}

bool TileService::loadAsync(uint32_t hash, uint32_t generation, const char *filename)
{
    if (service && service->hasAsync())
        return service->loadAsync(hash, generation, filename);
    if (backup && backup->hasAsync())
        return backup->loadAsync(hash, generation, filename);
    return false;
}

void TileService::tick(AsyncResultConsumer consumer)
{
    if (service)
        service->tick(consumer);
    if (backup)
        backup->tick(consumer);
}

void TileService::resetAsync()
{
    if (service)
        service->resetAsync();
    if (backup)
        backup->resetAsync();
}

TileService::~TileService()
{
    delete service;
    delete backup;
}
