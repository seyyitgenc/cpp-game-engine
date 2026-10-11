#pragma once

// Umbrella header: the bus plus every built-in event type.
//
// Intended wiring (not done yet, see the per-file comments):
//
//   - App owns one EventBus, reached through App::instance().
//   - GLFW callbacks Enqueue() input events alongside updating polled state;
//     framebuffer resize uses EnqueueCoalesced().
//   - HotReloadResourceManager::ApplyPendingReloads() enqueues a
//     ResourceReloadedEvent per rebuilt resource.
//   - The frame loop calls Flush() once, after glfwPollEvents() and before
//     update().
//   - The ImGui layer subscribes to input events at a high priority and
//     returns true while io.WantCaptureKeyboard / WantCaptureMouse, which is
//     what retires the normal/edit-mode callback swapping in callbacks.cpp.

#include "Event.hpp"
#include "EventBus.hpp"
#include "ApplicationEvents.hpp"
#include "InputEvents.hpp"
