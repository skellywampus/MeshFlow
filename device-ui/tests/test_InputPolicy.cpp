#include "input/policy/DefaultInputPolicyFactory.h"
#include "input/policy/IActionBindingResolver.h"
#include "input/policy/IInputContextProvider.h"
#include "input/policy/IUICommandDispatcher.h"
#include "input/policy/InputPipeline.h"
#include "input/policy/InputSourceRegistry.h"
#include "input/LauncherGridNavigation.h"
#include "lvgl.h"
#include <doctest/doctest.h>

namespace input_policy
{

namespace
{

class StaticContextProvider : public IInputContextProvider
{
  public:
    InputContextSnapshot getSnapshot() const override { return snapshot; }

    InputContextSnapshot snapshot{};
};

class RecordingDispatcher : public IUICommandDispatcher
{
  public:
    void dispatch(UICommand command, const CommandPayload &payload) override
    {
        lastCommand = command;
        lastPayload = payload;
        dispatchCount++;
    }

    UICommand lastCommand = UICommand::None;
    CommandPayload lastPayload{};
    int dispatchCount = 0;
};

} // namespace

TEST_CASE("DefaultInputPolicyFactory dispatches command actions")
{
    InputSourceRegistry registry;
    auto contextProvider = std::make_shared<StaticContextProvider>();
    auto dispatcher = std::make_shared<RecordingDispatcher>();

    DefaultInputPolicyFactory factory;
    auto buildResult = factory.build(registry, contextProvider, dispatcher);

    InputPipeline pipeline(buildResult.bindingResolver, contextProvider, dispatcher);
    pipeline.setPolicyChain(std::move(buildResult.chain));

    InputEvent event{};
    event.sourceId = "test";
    event.rawKeyCode = 0x101;
    event.resolvedKeyCode = 0x101;
    event.pressKind = PressKind::Press;

    std::vector<InputEvent> output;
    const bool forwarded = pipeline.process(event, InputCapabilities{}, output);

    CHECK_FALSE(forwarded);
    CHECK(output.empty());
    CHECK(event.action == InputAction::CommandHome);
    CHECK(dispatcher->dispatchCount == 1);
    CHECK(dispatcher->lastCommand == UICommand::GoHome);
    CHECK(dispatcher->lastPayload.command == UICommand::GoHome);
}

TEST_CASE("M9 Control key opens the existing Settings surface")
{
    InputSourceRegistry registry;
    auto contextProvider = std::make_shared<StaticContextProvider>();
    auto dispatcher = std::make_shared<RecordingDispatcher>();

    DefaultInputPolicyFactory factory;
    auto buildResult = factory.build(registry, contextProvider, dispatcher);
    InputPipeline pipeline(buildResult.bindingResolver, contextProvider, dispatcher);
    pipeline.setPolicyChain(std::move(buildResult.chain));

    InputEvent event{};
    event.sourceId = "thinknode-m9";
    event.rawKeyCode = 0x106;
    event.resolvedKeyCode = 0x106;
    event.pressKind = PressKind::Press;

    std::vector<InputEvent> output;
    const bool forwarded = pipeline.process(event, InputCapabilities{}, output);

    CHECK_FALSE(forwarded);
    CHECK(output.empty());
    CHECK(event.action == InputAction::CommandOpenSettings);
    CHECK(dispatcher->dispatchCount == 1);
    CHECK(dispatcher->lastCommand == UICommand::OpenSettings);
}

TEST_CASE("DefaultInputPolicyFactory still remaps focus traversal")
{
    InputSourceRegistry registry;
    auto contextProvider = std::make_shared<StaticContextProvider>();
    auto dispatcher = std::make_shared<RecordingDispatcher>();

    DefaultInputPolicyFactory factory;
    auto buildResult = factory.build(registry, contextProvider, dispatcher);

    InputPipeline pipeline(buildResult.bindingResolver, contextProvider, dispatcher);
    pipeline.setPolicyChain(std::move(buildResult.chain));

    InputEvent event{};
    event.sourceId = "test";
    event.rawKeyCode = 0x11;
    event.resolvedKeyCode = 0x11;
    event.pressKind = PressKind::Press;

    std::vector<InputEvent> output;
    const bool forwarded = pipeline.process(event, InputCapabilities{}, output);

    CHECK(forwarded);
    REQUIRE(output.size() == 1);
    CHECK(output.front().resolvedKeyCode == 11);
    CHECK(dispatcher->dispatchCount == 0);
}

TEST_CASE("FocusTraversalPolicy hands up/down to an open dropdown list only")
{
    InputSourceRegistry registry;
    auto contextProvider = std::make_shared<StaticContextProvider>();
    auto dispatcher = std::make_shared<RecordingDispatcher>();

    DefaultInputPolicyFactory factory;
    auto buildResult = factory.build(registry, contextProvider, dispatcher);

    InputPipeline pipeline(buildResult.bindingResolver, contextProvider, dispatcher);
    pipeline.setPolicyChain(std::move(buildResult.chain));

    auto forwardedKey = [&](uint32_t key) {
        InputEvent event{};
        event.sourceId = "test";
        event.rawKeyCode = key;
        event.resolvedKeyCode = key;
        std::vector<InputEvent> output;
        pipeline.process(event, InputCapabilities{}, output);
        REQUIRE(output.size() == 1);
        return output.front().resolvedKeyCode;
    };

    contextProvider->snapshot.focusSemantic = FocusSemantic::Dropdown;

    SUBCASE("open list keeps up/down")
    {
        contextProvider->snapshot.isEditMode = true;
        CHECK(forwardedKey(LV_KEY_UP) == LV_KEY_UP);
        CHECK(forwardedKey(LV_KEY_DOWN) == LV_KEY_DOWN);
    }

    SUBCASE("closed dropdown still traverses focus")
    {
        contextProvider->snapshot.isEditMode = false;
        CHECK(forwardedKey(LV_KEY_UP) == LV_KEY_PREV);
        CHECK(forwardedKey(LV_KEY_DOWN) == LV_KEY_NEXT);
    }
}

TEST_CASE("FocusTraversalPolicy keeps up/down for map panning and zooming but not for the map buttons")
{
    InputSourceRegistry registry;
    auto contextProvider = std::make_shared<StaticContextProvider>();
    auto dispatcher = std::make_shared<RecordingDispatcher>();

    DefaultInputPolicyFactory factory;
    auto buildResult = factory.build(registry, contextProvider, dispatcher);

    InputPipeline pipeline(buildResult.bindingResolver, contextProvider, dispatcher);
    pipeline.setPolicyChain(std::move(buildResult.chain));

    auto forwardedKey = [&](uint32_t key) {
        InputEvent event{};
        event.sourceId = "test";
        event.rawKeyCode = key;
        event.resolvedKeyCode = key;
        std::vector<InputEvent> output;
        pipeline.process(event, InputCapabilities{}, output);
        REQUIRE(output.size() == 1);
        return output.front().resolvedKeyCode;
    };

    SUBCASE("pan")
    {
        contextProvider->snapshot.focusSemantic = FocusSemantic::Map;
        CHECK(forwardedKey(LV_KEY_UP) == LV_KEY_UP);
        CHECK(forwardedKey(LV_KEY_DOWN) == LV_KEY_DOWN);
    }

    SUBCASE("zoom")
    {
        contextProvider->snapshot.focusSemantic = FocusSemantic::MapZoom;
        CHECK(forwardedKey(LV_KEY_UP) == LV_KEY_UP);
        CHECK(forwardedKey(LV_KEY_DOWN) == LV_KEY_DOWN);
    }

    SUBCASE("map buttons")
    {
        contextProvider->snapshot.focusSemantic = FocusSemantic::MapControls;
        CHECK(forwardedKey(LV_KEY_UP) == LV_KEY_PREV);
        CHECK(forwardedKey(LV_KEY_DOWN) == LV_KEY_NEXT);
    }
}

// Pins four-way navigation on the virtual keyboard: UP/DOWN must reach the buttonmatrix for row
// changes instead of being remapped to PREV/NEXT, which would move focus off the keyboard.
TEST_CASE("FocusTraversalPolicy passes all directional keys to the virtual keyboard")
{
    InputSourceRegistry registry;
    auto contextProvider = std::make_shared<StaticContextProvider>();
    auto dispatcher = std::make_shared<RecordingDispatcher>();

    DefaultInputPolicyFactory factory;
    auto buildResult = factory.build(registry, contextProvider, dispatcher);

    InputPipeline pipeline(buildResult.bindingResolver, contextProvider, dispatcher);
    pipeline.setPolicyChain(std::move(buildResult.chain));

    auto forwardedKey = [&](uint32_t key) {
        InputEvent event{};
        event.sourceId = "test";
        event.rawKeyCode = key;
        event.resolvedKeyCode = key;
        std::vector<InputEvent> output;
        pipeline.process(event, InputCapabilities{}, output);
        REQUIRE(output.size() == 1);
        return output.front().resolvedKeyCode;
    };

    contextProvider->snapshot.focusSemantic = FocusSemantic::Keyboard;
    CHECK(forwardedKey(LV_KEY_UP) == LV_KEY_UP);
    CHECK(forwardedKey(LV_KEY_DOWN) == LV_KEY_DOWN);
    CHECK(forwardedKey(LV_KEY_LEFT) == LV_KEY_LEFT);
    CHECK(forwardedKey(LV_KEY_RIGHT) == LV_KEY_RIGHT);
}

// Regression: a read-only modal with only Back must receive arrow keys,
// rather than silently cycling focus on that one button. Closing restores traversal.
TEST_CASE("Power statistics scroll context preserves arrows and restores menu navigation")
{
    InputSourceRegistry registry;
    auto context = std::make_shared<StaticContextProvider>();
    auto dispatcher = std::make_shared<RecordingDispatcher>();
    auto built = DefaultInputPolicyFactory{}.build(registry, context, dispatcher);
    InputPipeline pipeline(built.bindingResolver, context, dispatcher);
    pipeline.setPolicyChain(std::move(built.chain));
    auto key = [&](uint32_t value) {
        InputEvent event{};
        event.rawKeyCode = event.resolvedKeyCode = value;
        event.pressKind = PressKind::Press;
        std::vector<InputEvent> output;
        CHECK(pipeline.process(event, InputCapabilities{}, output));
        REQUIRE(output.size() == 1);
        return output.front().resolvedKeyCode;
    };
    context->snapshot.focusSemantic = FocusSemantic::Scrollable;
    CHECK(key(LV_KEY_UP) == LV_KEY_UP);
    CHECK(key(LV_KEY_DOWN) == LV_KEY_DOWN);
    CHECK(key(LV_KEY_ENTER) == LV_KEY_ENTER);
    CHECK(key(LV_KEY_ESC) == LV_KEY_ESC);
    context->snapshot.focusSemantic = FocusSemantic::Unknown;
    CHECK(key(LV_KEY_UP) == LV_KEY_PREV);
    CHECK(key(LV_KEY_DOWN) == LV_KEY_NEXT);
}

TEST_CASE("FocusTraversalPolicy dispatches directional launcher navigation")
{
    InputSourceRegistry registry;
    auto contextProvider = std::make_shared<StaticContextProvider>();
    auto dispatcher = std::make_shared<RecordingDispatcher>();

    DefaultInputPolicyFactory factory;
    auto buildResult = factory.build(registry, contextProvider, dispatcher);

    InputPipeline pipeline(buildResult.bindingResolver, contextProvider, dispatcher);
    pipeline.setPolicyChain(std::move(buildResult.chain));

    auto dispatchKey = [&](uint32_t key, InputAction expected) {
        InputEvent event{};
        event.sourceId = "test";
        event.rawKeyCode = key;
        event.resolvedKeyCode = key;
        std::vector<InputEvent> output;
        CHECK_FALSE(pipeline.process(event, InputCapabilities{}, output));
        CHECK(output.empty());
        CHECK(dispatcher->lastCommand == UICommand::LauncherNavigate);
        CHECK(dispatcher->lastPayload.argInt == static_cast<int32_t>(expected));
    };

    contextProvider->snapshot.focusSemantic = FocusSemantic::Launcher;
    dispatchKey(LV_KEY_UP, InputAction::NavigateUp);
    dispatchKey(LV_KEY_LEFT, InputAction::NavigateLeft);
    dispatchKey(LV_KEY_DOWN, InputAction::NavigateDown);
    dispatchKey(LV_KEY_RIGHT, InputAction::NavigateRight);
    CHECK(dispatcher->dispatchCount == 4);
}

TEST_CASE("Launcher grid navigation moves to the adjacent tile in each direction")
{
    using Direction = InputAction;
    CHECK(LauncherGridNavigation::nextIndex(0, Direction::NavigateUp) == 6);
    CHECK(LauncherGridNavigation::nextIndex(0, Direction::NavigateDown) == 2);
    CHECK(LauncherGridNavigation::nextIndex(0, Direction::NavigateLeft) == 1);
    CHECK(LauncherGridNavigation::nextIndex(0, Direction::NavigateRight) == 1);

    CHECK(LauncherGridNavigation::nextIndex(3, Direction::NavigateUp) == 1);
    CHECK(LauncherGridNavigation::nextIndex(3, Direction::NavigateDown) == 5);
    CHECK(LauncherGridNavigation::nextIndex(3, Direction::NavigateLeft) == 2);
    CHECK(LauncherGridNavigation::nextIndex(3, Direction::NavigateRight) == 2);

    CHECK(LauncherGridNavigation::nextIndex(5, Direction::NavigateUp) == 3);
    CHECK(LauncherGridNavigation::nextIndex(5, Direction::NavigateDown) == 7);
    CHECK(LauncherGridNavigation::nextIndex(5, Direction::NavigateLeft) == 4);
    CHECK(LauncherGridNavigation::nextIndex(5, Direction::NavigateRight) == 4);
    CHECK(LauncherGridNavigation::nextIndex(6, Direction::NavigateDown) == 0);
    CHECK(LauncherGridNavigation::nextIndex(7, Direction::NavigateDown) == 1);
    CHECK(LauncherGridNavigation::nextIndex(LauncherGridNavigation::TILE_COUNT, Direction::NavigateRight) == 1);
}

} // namespace input_policy
