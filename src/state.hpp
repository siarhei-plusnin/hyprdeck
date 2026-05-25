#pragma once

#include "constants.hpp"
#include "textinput.hpp"
#include <SharedDefs.hpp>
#include <desktop/DesktopTypes.hpp>
#include <event/EventBus.hpp>
#include <helpers/math/Math.hpp>
#include <render/Texture.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace hyprdeck {

    enum class EDragRow {
        NONE,
        NORMAL,
        SPECIAL,
    };

    enum class ESelectedRow {
        NORMAL,
        SPECIAL,
    };

    enum class EPromptMode {
        NONE,
        CREATE_SPECIAL,
        RENAME_SPECIAL,
    };

    enum class EWorkspaceCardAction {
        SWITCH,
    };

    enum class EInputMode {
        INACTIVE,
        OVERVIEW,
        NAMING,
        FILTER,
        CONFIRMATION,
        SHORTCUTS,
    };

    struct SWorkspaceCard {
        WORKSPACEID          id        = 0;
        CBox                 box       = {};
        PHLWORKSPACE         workspace = nullptr;
        std::string          label;
        bool                 special = false;
        EWorkspaceCardAction action  = EWorkspaceCardAction::SWITCH;
    };

    struct SSessionState {
        bool      active          = false;
        bool      zoomInitialized = false;
        MONITORID monitorID       = MONITOR_INVALID;
        double    zoom            = DEFAULT_ZOOM;
    };

    struct SInteractionState {
        bool     dragging                 = false;
        bool     suppressNextActiveCenter = false;
        bool     showSpecialTemplate      = false;
        double   dragStartCameraX         = 0.0;
        double   dragStartSpecialCameraX  = 0.0;
        Vector2D dragStart                = {};
        EDragRow dragRow                  = EDragRow::NONE;
    };

    struct SLayoutSignature {
        MONITORID                 monitorID                = MONITOR_INVALID;
        double                    transformedW             = 0.0;
        double                    transformedH             = 0.0;
        double                    pixelW                   = 0.0;
        double                    pixelH                   = 0.0;
        double                    zoom                     = DEFAULT_ZOOM;
        double                    cameraX                  = 0.0;
        double                    specialCameraX           = 0.0;
        bool                      resetCamera              = false;
        bool                      suppressNextActiveCenter = false;
        ESelectedRow              selectedRow              = ESelectedRow::NORMAL;
        WORKSPACEID               selectedNormalID         = 1;
        WORKSPACEID               selectedSpecialID        = WORKSPACE_INVALID;
        WORKSPACEID               pendingSpecialID         = WORKSPACE_INVALID;
        WORKSPACEID               activeNormalID           = WORKSPACE_INVALID;
        WORKSPACEID               activeSpecialID          = WORKSPACE_INVALID;
        WORKSPACEID               lastWorkspace            = 0;
        std::vector<WORKSPACEID>  normalWorkspaceIDs;
        std::vector<std::string>  specialWorkspaceKeys;
        std::string               workspaceFilter;
    };

    struct SLayoutState {
        bool                        dirty          = true;
        bool                        signatureValid = false;
        SLayoutSignature            signature;
        bool                        resetCamera    = false;
        double                      cameraX        = 0.0;
        double                      specialCameraX = 0.0;
        double                      stepX          = 1.0;
        double                      specialStepX   = 1.0;
        std::vector<SWorkspaceCard> cards;
        std::vector<SWorkspaceCard> specialCards;
    };

    struct SSelectionState {
        ESelectedRow selectedRow         = ESelectedRow::NORMAL;
        WORKSPACEID  selectedNormalID    = 1;
        WORKSPACEID  selectedSpecialID   = WORKSPACE_INVALID;
        WORKSPACEID  pendingSpecialID    = WORKSPACE_INVALID;
        WORKSPACEID  lastActiveNormalID  = WORKSPACE_INVALID;
        WORKSPACEID  lastActiveSpecialID = WORKSPACE_INVALID;
    };

    struct SNamingState {
        EPromptMode     promptMode              = EPromptMode::NONE;
        bool            promptCustomSelected    = false;
        size_t          namedSpecialPromptIndex = 0;
        STextInputState promptInput;
    };

    struct SWorkspaceFilterState {
        bool            promptOpen = false;
        std::string     text;
        std::string     previousText;
        STextInputState promptInput;
    };

    struct SConfirmationState {
        bool        open              = false;
        WORKSPACEID normalWorkspaceID = WORKSPACE_INVALID;
    };

    struct SShortcutMenuState {
        bool            open       = false;
        bool            sizeValid  = false;
        double          width      = 0.0;
        double          height     = 0.0;
        double          keyWidth   = 0.0;
        double          labelWidth = 0.0;
        double          descWidth  = 0.0;
        STextInputState searchInput;
    };

    struct SRenderCache {
        std::unordered_map<std::string, SP<Render::ITexture>> labelTextures;
        SP<Render::ITexture>                                  cursorTexture;
        Vector2D                                              cursorHotspot = {};
        Vector2D                                              cursorSize    = {18, 24};
    };

    struct SHookState {
        CHyprSignalListener renderHook;
        CHyprSignalListener mouseMoveHook;
        CHyprSignalListener mouseButtonHook;
        CHyprSignalListener mouseAxisHook;
        CHyprSignalListener keyboardHook;
    };

    struct SOverviewState {
        SSessionState                                          session;
        SInteractionState                                      interaction;
        SLayoutState                                           layout;
        SSelectionState                                        selection;
        SNamingState                                           naming;
        SWorkspaceFilterState                                  filter;
        SConfirmationState                                     confirmation;
        SShortcutMenuState                                     shortcuts;
        SRenderCache                                           renderCache;
        SHookState                                             hooks;
    };

    SOverviewState& state();
    EInputMode      currentInputMode();

} // namespace hyprdeck
