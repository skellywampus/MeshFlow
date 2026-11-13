#include "graphics/map/MapPanel.h"
#include "graphics/map/MapStartupView.h"
#include "graphics/map/BlockingQueue.h"
#include "graphics/map/TileLoader.h"
#include "graphics/map/TileService.h"
#include <atomic>
#include <chrono>
#include <doctest/doctest.h>
#include <thread>

class TestMapPanel : public MapPanel
{
  public:
    TestMapPanel(lv_obj_t *p, ITileService *s = nullptr) : MapPanel(p, s) {}
    void setHome(GeoPoint &p) { home = p; }
    void setCurrent(GeoPoint &p) { current = p; }
    void setScrolled(GeoPoint &p) { scrolled = p; }
    void redraw(void) { MapPanel::redraw(); }
    void center(void) { MapPanel::center(); }
    void setWidthPixel(int16_t width) { widthPixel = width; }
    void setHeightPixel(int16_t height) { heightPixel = height; }
    void setXStart(uint32_t x) { xStart = x; }
    void setYStart(uint32_t y) { yStart = y; }
    void setTilesX(uint32_t x) { tilesX = x; }
    void setTilesY(uint32_t y) { tilesY = y; }
    void setXOffset(int16_t x) { xOffset = x; }
    void setYOffset(int16_t y) { yOffset = y; }
    uint32_t getXStart() const { return xStart; }
    uint32_t getYStart() const { return yStart; }
    uint8_t getTilesX() const { return tilesX; }
    uint8_t getTilesY() const { return tilesY; }
    int16_t getXOffset() const { return xOffset; }
    int16_t getYOffset() const { return yOffset; }

    void redrawAll(void)
    {
        for (int i = 0; i <= tilesX; i++) {
            MapPanel::redraw();
        }
    }
};

class RawTileTestService : public ITileService
{
  public:
    explicit RawTileTestService(lv_image_dsc_t *result) : ITileService(nullptr), result(result) {}

    bool load(const char *, void *) override { return false; }
    lv_image_dsc_t *loadRaw(const char *) override
    {
        ++calls;
        return result;
    }

    lv_image_dsc_t *result;
    int calls = 0;
};

class CountingRawTileTestService : public ITileService
{
  public:
    CountingRawTileTestService() : ITileService(nullptr) {}

    bool load(const char *, void *) override { return false; }
    lv_image_dsc_t *loadRaw(const char *) override
    {
        calls.fetch_add(1, std::memory_order_relaxed);
        return nullptr;
    }

    std::atomic<uint32_t> calls{0};
};

TEST_CASE("MapPanel::scroll")
{
    TestMapPanel mapPanel(nullptr, nullptr);

    mapPanel.redrawAll();
    mapPanel.printTiles();

    SUBCASE("Scroll left by one pixel")
    {
        for (int i = 0; i < 256; i++) {
            CHECK(mapPanel.scroll(-1, 0, 256));
            mapPanel.printTiles();
        }
    }

    SUBCASE("Scroll right by one pixel")
    {
        for (int i = 0; i < 256; i++) {
            CHECK(mapPanel.scroll(1, 0, 256));
            mapPanel.printTiles();
        }
    }

    SUBCASE("Scroll up by one pixel")
    {
        for (int i = 0; i < 256; i++) {
            CHECK(mapPanel.scroll(0, -1, 256));
            mapPanel.printTiles();
        }
    }

    SUBCASE("Scroll down by one pixel")
    {
        for (int i = 0; i < 256; i++) {
            CHECK(mapPanel.scroll(0, 1, 256));
            mapPanel.printTiles();
        }
    }
}

TEST_CASE("MapPanel::update ignores unknown nodes")
{
    TestMapPanel mapPanel(nullptr, nullptr);

    CHECK_NOTHROW(mapPanel.update(1234, 51.5f, -0.12f));
    CHECK(mapPanel.getObjectsOnMap() == 0);
}

TEST_CASE("TileService background loading tries local raw tiles before fallback")
{
    lv_image_dsc_t localImage{};
    auto *local = new RawTileTestService(&localImage);
    auto *remote = new RawTileTestService(nullptr);
    TileService service(local);
    service.setBackupService(remote);

    CHECK(service.loadRaw("/maps/0/0/0.png") == &localImage);
    CHECK(local->calls == 1);
    CHECK(remote->calls == 0);
}

TEST_CASE("TileService background loading falls back when local raw tile is missing")
{
    lv_image_dsc_t remoteImage{};
    auto *local = new RawTileTestService(nullptr);
    auto *remote = new RawTileTestService(&remoteImage);
    TileService service(local);
    service.setBackupService(remote);

    CHECK(service.loadRaw("/maps/0/0/0.png") == &remoteImage);
    CHECK(local->calls == 1);
    CHECK(remote->calls == 1);
}

TEST_CASE("BlockingQueue enforces capacity and rejects work after stop")
{
    BlockingQueue<int> queue(1);

    CHECK(queue.tryPush(10));
    CHECK_FALSE(queue.tryPush(20));

    bool producerAccepted = false;
    std::thread producer([&queue, &producerAccepted] { producerAccepted = queue.push(20); });
    int value = 0;
    REQUIRE(queue.pop(value));
    CHECK(value == 10);
    producer.join();
    CHECK(producerAccepted);
    REQUIRE(queue.pop(value));
    CHECK(value == 20);

    queue.stop();
    CHECK_FALSE(queue.tryPush(30));
    CHECK_FALSE(queue.push(30));
    queue.reset();
    CHECK(queue.tryPush(40));
}

TEST_CASE("AsyncTileLoader shutdown releases a worker blocked by a full result queue")
{
    AsyncTileLoader loader;
    CountingRawTileTestService service;
    loader.start(&service);

    for (uint32_t i = 0; i < AsyncTileLoader::MAX_PENDING_RESULTS + 1; ++i)
        REQUIRE(loader.enqueue(i, 1, "/maps/0/0/0.png"));

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (service.calls.load(std::memory_order_relaxed) < AsyncTileLoader::MAX_PENDING_RESULTS + 1 &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    CHECK(service.calls.load(std::memory_order_relaxed) == AsyncTileLoader::MAX_PENDING_RESULTS + 1);

    // stop() must release the worker even if it is currently trying to deliver
    // the ninth result into an eight-slot queue that the UI no longer drains.
    loader.stop();
}

TEST_CASE("Map startup falls back when nodes have no positions")
{
    const auto selection = MapStartupView::fromNodeCoordinates({});

    CHECK_FALSE(selection.hasCenter);
    CHECK(selection.zoom == 3);
}

TEST_CASE("Map startup handles single and co-located node positions")
{
    const auto single = MapStartupView::fromNodeCoordinates({{123456789, -987654321}});
    CHECK(single.hasCenter);
    CHECK(single.latitude == doctest::Approx(12.3456789));
    CHECK(single.longitude == doctest::Approx(-98.7654321));
    CHECK(single.zoom == 13);

    const auto clustered = MapStartupView::fromNodeCoordinates({{400000000, -740000000}, {400000000, -740000000}});
    CHECK(clustered.hasCenter);
    CHECK(clustered.zoom == 13);
}

TEST_CASE("Map startup computes bounded zoom for separated nodes")
{
    const auto selection = MapStartupView::fromNodeCoordinates({{0, 0}, {100000000, 100000000}});

    CHECK(selection.hasCenter);
    CHECK(selection.latitude == doctest::Approx(5.0));
    CHECK(selection.longitude == doctest::Approx(5.0));
    CHECK(selection.zoom >= 3);
    CHECK(selection.zoom <= 17);
}

TEST_CASE("Map startup centers across the date line using the short longitude span")
{
    const auto selection = MapStartupView::fromNodeCoordinates({{0, 1795000000}, {0, -1795000000}});

    CHECK(selection.hasCenter);
    CHECK(std::abs(selection.longitude) == doctest::Approx(180.0));
    CHECK(selection.zoom == 17);
}
