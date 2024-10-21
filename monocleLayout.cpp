#include "monocleLayout.hpp"
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/desktop/Workspace.hpp>
#include <hyprland/src/helpers/MiscFunctions.hpp>
#include <hyprland/src/render/decorations/CHyprGroupBarDecoration.hpp>
#include <format>
#include <hyprland/src/render/decorations/IHyprWindowDecoration.hpp>


SMonocleNodeData* CHyprMonocleLayout::getNodeFromWindow(PHLWINDOW pWindow) {
    for (auto& nd : m_lMonocleNodesData) {
        if (nd.pWindow.lock() == pWindow)
            return &nd;
    }

    return nullptr;
}

int CHyprMonocleLayout::getNodesOnWorkspace(const int& ws) {
    int no = 0;
    for (auto& n : m_lMonocleNodesData) {
        if (n.workspaceID == ws)
            no++;
    }

    return no;
}

std::string CHyprMonocleLayout::getLayoutName() {
    return "Monocle";
}

void CHyprMonocleLayout::onWindowCreatedTiling(PHLWINDOW pWindow, eDirection direction) {
    if (pWindow->m_bIsFloating)
        return;

		const auto WSID = pWindow->workspaceID();
		
			
    const auto         PMONITOR = g_pCompositor->getMonitorFromID(pWindow->m_iMonitorID);

    auto               OPENINGON = g_pCompositor->m_pLastWindow.lock() && g_pCompositor->m_pLastWindow.lock()->m_pWorkspace == pWindow->m_pWorkspace ? g_pCompositor->m_pLastWindow.lock() : nullptr;

		const auto				MOUSECOORDS = g_pInputManager->getMouseCoordsInternal();
		



		if (g_pInputManager->m_bWasDraggingWindow && OPENINGON) {
			if (pWindow->checkInputOnDecos(INPUT_TYPE_DRAG_END, MOUSECOORDS, pWindow))
				return;
		}

    if (OPENINGON && OPENINGON != pWindow && OPENINGON->m_sGroupData.pNextWindow.lock() // target is group
        && pWindow->canBeGroupedInto(OPENINGON)) {


        static const auto* USECURRPOS = (Hyprlang::INT* const*)g_pConfigManager->getConfigValuePtr("group:insert_after_current");
        (**USECURRPOS ? OPENINGON : OPENINGON->getGroupTail())->insertWindowToGroup(pWindow);

        OPENINGON->setGroupCurrent(pWindow);
        pWindow->applyGroupRules();
        pWindow->updateWindowDecos();
        recalculateWindow(pWindow);
        if(!pWindow->getDecorationByType(DECORATION_GROUPBAR))
			      pWindow->addWindowDeco(std::make_unique<CHyprGroupBarDecoration>(pWindow));

        return;
    }

    pWindow->applyGroupRules();
    const auto PNODE = &m_lMonocleNodesData.emplace_front();
	  PNODE->workspaceID = pWindow->workspaceID();
	  PNODE->pWindow = pWindow;
    if (g_pCompositor->getWorkspaceByID(WSID)->m_bHasFullscreenWindow) {
	      g_pCompositor->setWindowFullscreenInternal(g_pCompositor->getFullscreenWindowOnWorkspace(pWindow->workspaceID()), FSMODE_FULLSCREEN);
    }

    recalculateMonitor(pWindow->m_iMonitorID);
	  g_pCompositor->setWindowFullscreenInternal(pWindow, FSMODE_MAXIMIZED);
	  g_pCompositor->focusWindow(pWindow);
}

void CHyprMonocleLayout::onWindowRemovedTiling(PHLWINDOW pWindow) {
    const auto PNODE = getNodeFromWindow(pWindow);

    if (!PNODE)
        return;

    pWindow->unsetWindowData(PRIORITY_LAYOUT);
    pWindow->updateWindowData();


    if (pWindow->isFullscreen())
        g_pCompositor->setWindowFullscreenInternal(pWindow,FSMODE_FULLSCREEN);

    m_lMonocleNodesData.remove(*PNODE);

    recalculateMonitor(pWindow->m_iMonitorID);
	  
}

void CHyprMonocleLayout::recalculateMonitor(const MONITORID& monid) {
    const auto PMONITOR   = g_pCompositor->getMonitorFromID(monid);
    const auto PWORKSPACE = PMONITOR->activeWorkspace;

    if (!PWORKSPACE)
        return;

    g_pHyprRenderer->damageMonitor(PMONITOR);

    if (PMONITOR->activeSpecialWorkspace) {
        calculateWorkspace(PMONITOR->activeSpecialWorkspace);
    }

    // calc the WS
    calculateWorkspace(PWORKSPACE);
}

void CHyprMonocleLayout::calculateWorkspace(PHLWORKSPACE PWORKSPACE) {
    if (!PWORKSPACE)
        return;

    const auto         PMONITOR = g_pCompositor->getMonitorFromID(PWORKSPACE->m_iMonitorID);
    if (PWORKSPACE->m_bHasFullscreenWindow) {
        if (PWORKSPACE->m_efFullscreenMode == FSMODE_FULLSCREEN)
            return;

        // massive hack from the fullscreen func
        const auto      PFULLWINDOW = g_pCompositor->getFullscreenWindowOnWorkspace(PWORKSPACE->m_iID);

        SMonocleNodeData fakeNode;
        fakeNode.pWindow         = PFULLWINDOW;
        fakeNode.position        = PMONITOR->vecPosition + PMONITOR->vecReservedTopLeft;
        fakeNode.size            = PMONITOR->vecSize - PMONITOR->vecReservedTopLeft - PMONITOR->vecReservedBottomRight;
        fakeNode.workspaceID     = PWORKSPACE->m_iID;
        PFULLWINDOW->m_vPosition = fakeNode.position;
        PFULLWINDOW->m_vSize     = fakeNode.size;

        applyNodeDataToWindow(&fakeNode);

        return;
    }

	  for(auto &md : m_lMonocleNodesData) {
        if (md.workspaceID != PWORKSPACE->m_iID)
			    continue;
		   	md.position = PMONITOR->vecPosition  + PMONITOR->vecReservedTopLeft + Vector2D(0.0f, 0.0f);
		    md.size = Vector2D(PMONITOR->vecSize.x - PMONITOR->vecReservedBottomRight.x - PMONITOR->vecReservedTopLeft.x, PMONITOR->vecSize.y - PMONITOR->vecReservedBottomRight.y - PMONITOR->vecReservedTopLeft.y);
		    applyNodeDataToWindow(&md);
  	}
}

void CHyprMonocleLayout::applyNodeDataToWindow(SMonocleNodeData* pNode) {
    PHLMONITOR PMONITOR = nullptr;

    if (g_pCompositor->isWorkspaceSpecial(pNode->workspaceID)) {
        for (auto& m : g_pCompositor->m_vMonitors) {
            if (m->activeSpecialWorkspaceID() == pNode->workspaceID) {
                PMONITOR = m;
                break;
            }
        }
    } else {
        PMONITOR = g_pCompositor->getMonitorFromID(g_pCompositor->getWorkspaceByID(pNode->workspaceID)->m_iMonitorID);
    }

    if (!PMONITOR) {
        Debug::log(ERR, "Orphaned Node {} (workspace ID: {})!!", static_cast<void *>(pNode), pNode->workspaceID);
        return;
    }

    // for gaps outer
    const bool DISPLAYLEFT   = STICKS(pNode->position.x, PMONITOR->vecPosition.x + PMONITOR->vecReservedTopLeft.x);
    const bool DISPLAYRIGHT  = STICKS(pNode->position.x + pNode->size.x, PMONITOR->vecPosition.x + PMONITOR->vecSize.x - PMONITOR->vecReservedBottomRight.x);
    const bool DISPLAYTOP    = STICKS(pNode->position.y, PMONITOR->vecPosition.y + PMONITOR->vecReservedTopLeft.y);
    const bool DISPLAYBOTTOM = STICKS(pNode->position.y + pNode->size.y, PMONITOR->vecPosition.y + PMONITOR->vecSize.y - PMONITOR->vecReservedBottomRight.y);

    const auto PWINDOW = pNode->pWindow.lock();
		const auto WORKSPACERULE = g_pConfigManager->getWorkspaceRuleFor(g_pCompositor->getWorkspaceByID(PWINDOW->workspaceID()));

		if (PWINDOW->isFullscreen() && !pNode->ignoreFullscreenChecks)
			return;

    PWINDOW->unsetWindowData(PRIORITY_LAYOUT);
    PWINDOW->updateWindowData();
		

		


		static auto* const PANIMATE = (Hyprlang::INT* const*)g_pConfigManager->getConfigValuePtr("misc:animate_manual_resizes");

    static auto* const PGAPSINDATA     = (Hyprlang::CUSTOMTYPE* const*)g_pConfigManager->getConfigValuePtr("general:gaps_in");
    static auto* const PGAPSOUTDATA    = (Hyprlang::CUSTOMTYPE* const*)g_pConfigManager->getConfigValuePtr("general:gaps_out");
    auto* const        PGAPSIN         = (CCssGapData*)(*PGAPSINDATA)->getData();
    auto* const        PGAPSOUT        = (CCssGapData*)(*PGAPSOUTDATA)->getData();

    auto               gapsIn  = WORKSPACERULE.gapsIn.value_or(*PGAPSIN);
    auto               gapsOut = WORKSPACERULE.gapsOut.value_or(*PGAPSOUT);



    if (!validMapped(PWINDOW)) {
        Debug::log(ERR, "Node {} holding invalid window {}!!", pNode, PWINDOW);
        return;
    }


    PWINDOW->m_vSize     = pNode->size;
    PWINDOW->m_vPosition = pNode->position;

    //auto calcPos  = PWINDOW->m_vPosition + Vector2D(*PBORDERSIZE, *PBORDERSIZE);
    //auto calcSize = PWINDOW->m_vSize - Vector2D(2 * *PBORDERSIZE, 2 * *PBORDERSIZE);

    auto       calcPos  = PWINDOW->m_vPosition;
    auto       calcSize = PWINDOW->m_vSize;

    const auto OFFSETTOPLEFT = Vector2D((double)(DISPLAYLEFT ? gapsOut.left : gapsIn.left), (double)(DISPLAYTOP ? gapsOut.top : gapsIn.top));

    const auto OFFSETBOTTOMRIGHT = Vector2D((double)(DISPLAYRIGHT ? gapsOut.right : gapsIn.right), (double)(DISPLAYBOTTOM ? gapsOut.bottom : gapsIn.bottom));

    calcPos  = calcPos + OFFSETTOPLEFT;
    calcSize = calcSize - OFFSETTOPLEFT - OFFSETBOTTOMRIGHT;

    const auto RESERVED = PWINDOW->getFullWindowReservedArea();
    calcPos             = calcPos + RESERVED.topLeft;
    calcSize            = calcSize - (RESERVED.topLeft + RESERVED.bottomRight);

		CBox wb = {calcPos, calcSize};
		wb.round();
      PWINDOW->m_vRealSize     = wb.size(); 
      PWINDOW->m_vRealPosition = wb.pos(); 
      g_pXWaylandManager->setWindowSize(PWINDOW, calcSize);

    if (m_bForceWarps && !**PANIMATE) {
        g_pHyprRenderer->damageWindow(PWINDOW);

        PWINDOW->m_vRealPosition.warp();
        PWINDOW->m_vRealSize.warp();

        g_pHyprRenderer->damageWindow(PWINDOW);
    }

    PWINDOW->updateWindowDecos();

}


void CHyprMonocleLayout::resizeActiveWindow(const Vector2D& pixResize, eRectCorner corner, PHLWINDOW pWindow) {
	return;
}

void CHyprMonocleLayout::fullscreenRequestForWindow(PHLWINDOW pWindow, const eFullscreenMode CURRENT_EFFECTIVE_MODE, const eFullscreenMode EFFECTIVE_MODE) {
    const auto PMONITOR   = g_pCompositor->getMonitorFromID(pWindow->m_iMonitorID);
    const auto PWORKSPACE = pWindow->m_pWorkspace;

    // save position and size if floating
    if (pWindow->m_bIsFloating && CURRENT_EFFECTIVE_MODE == FSMODE_NONE) {
        pWindow->m_vLastFloatingSize     = pWindow->m_vRealSize.goal();
        pWindow->m_vLastFloatingPosition = pWindow->m_vRealPosition.goal();
        pWindow->m_vPosition             = pWindow->m_vRealPosition.goal();
        pWindow->m_vSize                 = pWindow->m_vRealSize.goal();
    }

    if (EFFECTIVE_MODE == FSMODE_NONE) {
        // if it got its fullscreen disabled, set back its node if it had one
        const auto PNODE = getNodeFromWindow(pWindow);
        if (PNODE)
            applyNodeDataToWindow(PNODE);
        else {
            // get back its' dimensions from position and size
            pWindow->m_vRealPosition = pWindow->m_vLastFloatingPosition;
            pWindow->m_vRealSize     = pWindow->m_vLastFloatingSize;

            pWindow->unsetWindowData(PRIORITY_LAYOUT);
            pWindow->updateWindowData();
        }
    } else {
        // apply new pos and size being monitors' box
        if (EFFECTIVE_MODE == FSMODE_FULLSCREEN) {
            pWindow->m_vRealPosition = PMONITOR->vecPosition;
            pWindow->m_vRealSize     = PMONITOR->vecSize;
        } else {
            // This is a massive hack.
            // We make a fake "only" node and apply
            // To keep consistent with the settings without C+P code

            SMonocleNodeData fakeNode;
            fakeNode.pWindow                = pWindow;
            fakeNode.position               = PMONITOR->vecPosition + PMONITOR->vecReservedTopLeft;
            fakeNode.size                   = PMONITOR->vecSize - PMONITOR->vecReservedTopLeft - PMONITOR->vecReservedBottomRight;
            fakeNode.workspaceID            = pWindow->workspaceID();
            pWindow->m_vPosition            = fakeNode.position;
            pWindow->m_vSize                = fakeNode.size;
            fakeNode.ignoreFullscreenChecks = true;

            applyNodeDataToWindow(&fakeNode);
        }
    }

    //g_pCompositor->changeWindowZOrder(pWindow, true);
}

void CHyprMonocleLayout::onWindowFocusChange(PHLWINDOW pNewFocus) {
	IHyprLayout::onWindowFocusChange(pNewFocus);
	fullscreenRequestForWindow(pNewFocus, FSMODE_MAXIMIZED, FSMODE_MAXIMIZED);
}



bool CHyprMonocleLayout::isWindowTiled(PHLWINDOW pWindow) {
	return true;
}


void CHyprMonocleLayout::recalculateWindow(PHLWINDOW pWindow) {
    const auto PNODE = getNodeFromWindow(pWindow);

    if (!PNODE)
        return;

    recalculateMonitor(pWindow->m_iMonitorID);
}

SWindowRenderLayoutHints CHyprMonocleLayout::requestRenderHints(PHLWINDOW pWindow) {
    // window should be valid, insallah

    SWindowRenderLayoutHints hints;

    return hints; // master doesnt have any hints
}

void CHyprMonocleLayout::switchWindows(PHLWINDOW pWindow, PHLWINDOW pWindow2) {
    // windows should be valid, insallah

    const auto PNODE  = getNodeFromWindow(pWindow);
    const auto PNODE2 = getNodeFromWindow(pWindow2);

	  Debug::log(LOG, "SWITCH WINDOWS {} {}", pWindow, pWindow2);
    if (!PNODE2 || !PNODE)
        return;

	  Debug::log(LOG, "YAY");
    const auto inheritFullscreen = prepareLoseFocus(pWindow);

    if (PNODE->workspaceID != PNODE2->workspaceID) {
        std::swap(pWindow2->m_iMonitorID, pWindow->m_iMonitorID);
        std::swap(pWindow2->m_pWorkspace, pWindow->m_pWorkspace);
    }

    // massive hack: just swap window pointers, lol
    PNODE->pWindow  = pWindow2;
    PNODE2->pWindow = pWindow;

    recalculateMonitor(pWindow->m_iMonitorID);
    if (PNODE2->workspaceID != PNODE->workspaceID)
        recalculateMonitor(pWindow2->m_iMonitorID);

    g_pHyprRenderer->damageWindow(pWindow);
    g_pHyprRenderer->damageWindow(pWindow2);

    prepareNewFocus(pWindow2, inheritFullscreen);
}

void CHyprMonocleLayout::alterSplitRatio(PHLWINDOW pWindow, float ratio, bool exact) {
    // window should be valid, insallah

	  return;
}


bool CHyprMonocleLayout::prepareLoseFocus(PHLWINDOW pWindow) {
	  Debug::log(LOG, "PREPARE LOSE FOCUS {}", pWindow);
	  return true;
}

void CHyprMonocleLayout::prepareNewFocus(PHLWINDOW pWindow, bool inheritFullscreen) {
    if (!pWindow)
        return;

	  Debug::log(LOG, "PREPARE NEW FOCUS {}", pWindow);
    if (inheritFullscreen)
        g_pCompositor->setWindowFullscreenInternal(pWindow, g_pCompositor->getWorkspaceByID(pWindow->workspaceID())->m_efFullscreenMode);
}

std::any CHyprMonocleLayout::layoutMessage(SLayoutMessageHeader header, std::string message) {
    return 0;
}


void CHyprMonocleLayout::moveWindowTo(PHLWINDOW pWindow, const std::string& dir, bool silent) {
    if (!isDirection(dir))
        return;

    const auto PWINDOW2 = g_pCompositor->getWindowInDirection(pWindow, dir[0]);

	  if (!PWINDOW2)
		    return;

	  pWindow->setAnimationsToMove();
	  

		if (pWindow->m_pWorkspace != PWINDOW2->m_pWorkspace) {
 			// if different monitors, send to monitor
			onWindowRemovedTiling(pWindow);
			pWindow->moveToWorkspace(PWINDOW2->m_pWorkspace);
			pWindow->m_iMonitorID = PWINDOW2->m_iMonitorID;
			if (!silent) {
				const auto pMonitor = g_pCompositor->getMonitorFromID(pWindow->m_iMonitorID);
				g_pCompositor->setActiveMonitor(pMonitor);
			}
			onWindowCreatedTiling(pWindow);
		}
}


void CHyprMonocleLayout::replaceWindowDataWith(PHLWINDOW from, PHLWINDOW to) {
    const auto PNODE = getNodeFromWindow(from);

    if (!PNODE)
        return;

    PNODE->pWindow = to;

    applyNodeDataToWindow(PNODE);
}

void CHyprMonocleLayout::onEnable() {
    for (auto& w : g_pCompositor->m_vWindows) {
        if (w->m_bIsFloating || !w->m_bIsMapped || w->isHidden())
            continue;

        onWindowCreatedTiling(w);
    }
}

void CHyprMonocleLayout::onDisable() {
    m_lMonocleNodesData.clear();
}


Vector2D CHyprMonocleLayout::predictSizeForNewWindowTiled() {
	//What the fuck is this shit. Seriously.
	return {};
}


