#pragma once

#include "globals.hpp"
#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/layout/IHyprLayout.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <vector>
#include <list>
#include <deque>
#include <any>

enum eFullscreenMode : int8_t;

struct SMonocleNodeData {
    PHLWINDOWREF pWindow;

    Vector2D position;
    Vector2D size;

    int      workspaceID = -1;
		bool		 ignoreFullscreenChecks = false;

    bool     operator==(const SMonocleNodeData& rhs) const {
        return pWindow.lock() == rhs.pWindow.lock();
    }
};

class CHyprMonocleLayout : public IHyprLayout {
  public:
    virtual void                     onWindowCreatedTiling(PHLWINDOW, eDirection direction = DIRECTION_DEFAULT);
    virtual void                     onWindowRemovedTiling(PHLWINDOW);
    virtual bool                     isWindowTiled(PHLWINDOW);
    virtual void                     recalculateMonitor(const int&);
    virtual void                     recalculateWindow(PHLWINDOW);
    virtual void                     resizeActiveWindow(const Vector2D&, eRectCorner corner, PHLWINDOW pWindow = nullptr);
    virtual void                     fullscreenRequestForWindow(PHLWINDOW, eFullscreenMode, bool);
    virtual std::any                 layoutMessage(SLayoutMessageHeader, std::string);
    virtual SWindowRenderLayoutHints requestRenderHints(PHLWINDOW);
    virtual void                     switchWindows(PHLWINDOW, PHLWINDOW);
		virtual void										 moveWindowTo(PHLWINDOW, const std::string& dir, bool silent);
    virtual void                     alterSplitRatio(PHLWINDOW, float, bool);
    virtual std::string              getLayoutName();
    virtual void                     replaceWindowDataWith(PHLWINDOW from, PHLWINDOW to);
    virtual Vector2D 								 predictSizeForNewWindowTiled();

    virtual void                     onEnable();
    virtual void                     onDisable();
	  virtual void                     onWindowFocusChange(PHLWINDOW);

		void 														removeWorkspaceData(const int& ws);

  private:
    std::list<SMonocleNodeData>        m_lMonocleNodesData;

    bool                              m_bForceWarps = false;

    int                               getNodesOnWorkspace(const int&);
    void                              applyNodeDataToWindow(SMonocleNodeData*);
    void                              resetNodeSplits(const int&);
    SMonocleNodeData*                  getNodeFromWindow(PHLWINDOW);
    SMonocleNodeData*                  getMasterNodeOnWorkspace(const int&);
    void                              calculateWorkspace(PHLWORKSPACE);
    PHLWINDOW                          getNextWindow(PHLWINDOW, bool);
    int                               getMastersOnWorkspace(const int&);
    bool                              prepareLoseFocus(PHLWINDOW);
    void                              prepareNewFocus(PHLWINDOW, bool inherit_fullscreen);

    friend struct SMonocleNodeData;
};


template <typename CharT>
struct std::formatter<SMonocleNodeData*, CharT> : std::formatter<CharT> {
    template <typename FormatContext>
    auto format(const SMonocleNodeData* const& node, FormatContext& ctx) const {
        auto out = ctx.out();
        if (!node)
            return std::format_to(out, "[Node nullptr]");
        std::format_to(out, "[Node {:x}: workspace: {}, pos: {:j2}, size: {:j2}", (uintptr_t)node, node->workspaceID, node->position, node->size);
        if (!node->pWindow.expired())
            std::format_to(out, ", window: {:x}", node->pWindow.lock());
        return std::format_to(out, "]");
    }
};
