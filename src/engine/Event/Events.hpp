#pragma once

// Umbrella header: the bus plus every built-in event type.
//
// Wiring:
//
//   - App owns the one EventBus and hands it to whatever publishes or
//     listens (Window, CameraController).
//   - Window's GLFW callbacks Enqueue() window and input events, including
//     the drags and double clicks they detect; framebuffer resize uses
//     EnqueueCoalesced(). Held-key state is built from KeyPressed/KeyReleased
//     by whoever needs it (CameraController).
//   - App::Run() enqueues a FrameEvent and calls Flush() once per drawn
//     frame, after Window::PollEvents() and before anything updates. Esc
//     enqueues a QuitApplicationEvent, which App answers by closing the
//     window.
//   - The Scene and Inspector panels enqueue CreateEntityEvent,
//     AddComponentEvent and RemoveComponentEvent, which App applies.
//
// Not wired yet:
//
//   - HotReloadResourceManager::ApplyPendingReloads() should enqueue a
//     ResourceReloadedEvent per rebuilt resource.
//   - An ImGui layer would subscribe to input events at a high priority and
//     return true while io.WantCaptureKeyboard / WantCaptureMouse, so the
//     camera never sees input meant for the UI.

#include "Event.hpp"
#include "EventBus.hpp"
#include "ApplicationEvents.hpp"
#include "InputEvents.hpp"
#include "SceneEvents.hpp"
