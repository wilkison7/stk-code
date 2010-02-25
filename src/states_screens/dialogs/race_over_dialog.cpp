//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009 Marianne Gagnon
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "challenges/unlock_manager.hpp"
#include "guiengine/engine.hpp"
#include "guiengine/widgets.hpp"
#include "io/file_manager.hpp"
#include "input/input_manager.hpp"
#include "karts/kart.hpp"
#include "modes/three_strikes_battle.hpp"
#include "modes/world.hpp"
#include "network/network_manager.hpp"
#include "race/race_manager.hpp"
#include "states_screens/dialogs/race_over_dialog.hpp"
#include "states_screens/kart_selection.hpp"
#include "states_screens/main_menu_screen.hpp"
#include "states_screens/state_manager.hpp"
#include "utils/string_utils.hpp"
#include "utils/translation.hpp"

using namespace GUIEngine;
using namespace irr::core;
using namespace irr::gui;
using namespace irr::video;

// ------------------------------------------------------------------------------------------------------

RaceOverDialog::RaceOverDialog(const float percentWidth, 
                               const float percentHeight) 
              : ModalDialog(percentWidth, percentHeight)
{
    // Switch to barrier mode: server waits for ack from each client
    network_manager->beginRaceResultBarrier();

    //std::cout << "race_manager->getMajorMode()=" << race_manager->getMajorMode()
    //          << ", RaceManager::MAJOR_MODE_GRAND_PRIX=" << RaceManager::MAJOR_MODE_GRAND_PRIX 
    //          << ", RaceManager::MAJOR_MODE_SINGLE=" << RaceManager::MAJOR_MODE_SINGLE << "\n";
        
    const bool show_highscores = (race_manager->getMajorMode() != RaceManager::MAJOR_MODE_GRAND_PRIX) &&
                                 World::getWorld()->useHighScores();
    
    const int text_height = GUIEngine::getFontHeight();
    
    const int button_h = text_height + 6;
    const int margin_between_buttons = 12;
    m_buttons_y_from = m_area.getHeight() - 3*(button_h + margin_between_buttons);
    
    // ---- Ranking
    core::rect< s32 > area(0, 0, (show_highscores ? m_area.getWidth()*2/3 : m_area.getWidth()), text_height);
    IGUIStaticText* caption = GUIEngine::getGUIEnv()->addStaticText( _("Race Results"),
                                                              area, false, false, // border, word warp
                                                              m_irrlicht_window);
    caption->setTabStop(false);
    caption->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
    
    World *world = World::getWorld();
    const unsigned int num_karts = world->getNumKarts();
    int*  order  = new int [num_karts];
    for (unsigned int n=0; n<num_karts; n++) order[n] = -1;
    world->raceResultOrder(order);
        
    const bool display_time = (world->getClockMode() == WorldStatus::CLOCK_CHRONO);

    int line_h = text_height + 15;
    const int lines_from_y = text_height + 15;
    
    // make things more compact if we're missing space
    while (lines_from_y + (int)num_karts*line_h > m_buttons_y_from) // cheap way to avoid calculating the               
    {                                                             // required size with proper maths
        line_h = (int)(line_h*0.9f);
    }
    
    m_rankings_y_bottom = -1.0f;
    
    int kart_id = 0; // 'i' below is not reliable because some karts (e.g. leader) will be skipped
    for (unsigned int i = 0; i < num_karts; ++i)
    {
        if (order[i] == -1) continue;
        
        stringw kart_results_line;
        
        const Kart *current_kart = world->getKart(order[i]);
        const stringw& kart_name = current_kart->getName();
                
        char sTime[20];
        sTime[0]=0;
        
        const float time      = current_kart->getFinishTime();
        
        if (display_time)
        {
            const int min     = (int) floor ( time / 60.0 ) ;
            const int sec     = (int) floor ( time - (double) ( 60 * min ) ) ;
            const int tenths  = (int) floor ( 10.0f * (time - (double)(sec + 60* min)));
            sprintf ( sTime, "%d:%02d:%d", min,  sec,  tenths ) ;
        }
        
        //This shows position + driver name + time + points earned + total points
        if (race_manager->getMajorMode() == RaceManager::MAJOR_MODE_GRAND_PRIX)
        {
            const int prev_score = race_manager->getKartPrevScore(order[i]);
            const int new_score = race_manager->getKartScore(order[i]);
            
            if (display_time)
            {
                kart_results_line = StringUtils::insertValues( L"#%i. %s %s (%i + %i = %i)",
                                                              current_kart->getPosition(), sTime,
                                                              kart_name.c_str(), prev_score,
                                                              (new_score - prev_score), new_score);
            }
            else
            {
                kart_results_line = StringUtils::insertValues( L"#%i. %s (%i + %i = %i)",
                                                              current_kart->getPosition(), kart_name.c_str(),
                                                              prev_score, (new_score - prev_score), new_score);
            }
        }
        else if (race_manager->getMinorMode() == RaceManager::MINOR_MODE_3_STRIKES)
        {
            //I18N: In results of battle mode; %i is where the rank is inserted;
            //I18N: the first %s is where the kart name is inserted;
            //I18N: the second %s is where the time is inserted
            kart_results_line = StringUtils::insertValues( _("%i. %s : survived for %s"),
                                                          current_kart->getPosition(),
                                                          kart_name.c_str(), sTime);
        }
        else
        {
            kart_results_line = StringUtils::insertValues( L"%i. %s %s",
                                                          current_kart->getPosition(),
                                                          kart_name.c_str(), sTime);
        }
        
        const KartProperties* prop = current_kart->getKartProperties();
        std::string icon_path = file_manager->getDataDir() ;
        icon_path += "/karts/" + prop->getIdent() + "/" + prop->getIconFile();
        ITexture* kart_icon_texture = irr_driver->getTexture( icon_path );
        
        const int entry_width = (show_highscores?  m_area.getWidth()*2/3 :  m_area.getWidth());
        
        const int icon_size = text_height;
        core::rect< s32 > entry_area(10 + icon_size, lines_from_y + line_h*i,
                                     entry_width   , lines_from_y + line_h*(i+1));
        core::rect< s32 > icon_area (5             , lines_from_y + line_h*i,
                                     5+icon_size   , lines_from_y + line_h*i + icon_size);
        
        m_rankings_y_bottom = lines_from_y + line_h*(i+1);
        
        GUIEngine::getGUIEnv()->addStaticText( kart_results_line.c_str(), entry_area,
                                               false , true , // border, word warp
                                               m_irrlicht_window);
        IGUIImage* img = GUIEngine::getGUIEnv()->addImage( icon_area, m_irrlicht_window );
        img->setImage(kart_icon_texture);
        img->setScaleImage(true);
        img->setTabStop(false);
        img->setUseAlphaChannel(true);
        
        kart_id++;
    }

    delete[] order;
    
    // ---- Highscores
    if (show_highscores)
    {
        const HighscoreEntry *hs = World::getWorld()->getHighscores();
        if (hs != NULL)
        {
            core::rect< s32 > hsarea(m_area.getWidth()*2/3, 0, m_area.getWidth(), text_height);
            IGUIStaticText* highscores = GUIEngine::getGUIEnv()->addStaticText( _("Highscores"),
                                                            hsarea, false, false, // border, word warp
                                                            m_irrlicht_window);
            highscores->setTabStop(false);
            highscores->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
            
            unsigned int num_scores = hs->getNumberEntries();
                    
            char timebuffer[64];
            for (unsigned int i=0; i<num_scores; i++)
            {            
                std::string kart_name, name;
                float T;
                hs->getEntry(i, kart_name, name, &T);
                const int   MINS   = (int) floor ( T / 60.0 ) ;
                const int   SECS   = (int) floor ( T - (float) ( 60 * MINS ) ) ;
                const int   TENTHS = (int) floor ( 10.0f * (T - (float)(SECS + 60*MINS)));
                sprintf(timebuffer, "%2d:%02d.%01d", MINS, SECS, TENTHS);
                            
                const int line_from = lines_from_y + text_height*(i*3);
                
                stringw playerName = name.c_str();
                
                core::rect< s32 > linearea(m_area.getWidth()*2/3, line_from,
                                           m_area.getWidth(), line_from + text_height);
                GUIEngine::getGUIEnv()->addStaticText( playerName.c_str(),
                                                       linearea, false, false, // border, word warp
                                                       m_irrlicht_window);
                core::rect< s32 > linearea2(m_area.getWidth()*2/3, line_from + text_height,
                                           m_area.getWidth(), line_from + text_height*2);
                GUIEngine::getGUIEnv()->addStaticText( stringw(timebuffer).c_str(),
                                                      linearea2, false, false, // border, word warp
                                                      m_irrlicht_window);
            } // next score
        } // end if hs != NULL
    } // end if not GP
    
    // ---- Buttons at the bottom
    if (race_manager->getMajorMode() == RaceManager::MAJOR_MODE_SINGLE)
    {
        ButtonWidget* new_race_btn = new ButtonWidget();
        new_race_btn->m_properties[PROP_ID] = "newracebtn";
        new_race_btn->x = 15;
        new_race_btn->y = m_area.getHeight() - (button_h + margin_between_buttons)*3;
        new_race_btn->w = m_area.getWidth() - 30;
        new_race_btn->h = button_h;
        new_race_btn->m_text = _("Setup New Race");
        new_race_btn->setParent(m_irrlicht_window);
        m_children.push_back(new_race_btn);
        new_race_btn->add();
    
        ButtonWidget* race_again_btn = new ButtonWidget();
        race_again_btn->m_properties[PROP_ID] = "raceagainbtn";
        race_again_btn->x = 15;
        race_again_btn->y = m_area.getHeight() - (button_h + margin_between_buttons)*2;
        race_again_btn->w = m_area.getWidth() - 30;
        race_again_btn->h = button_h;
        race_again_btn->m_text = _("Race in this track again");
        race_again_btn->setParent(m_irrlicht_window);
        m_children.push_back(race_again_btn);
        race_again_btn->add();
        
        ButtonWidget* whats_next_btn = new ButtonWidget();
        whats_next_btn->x = 15;
        whats_next_btn->y = m_area.getHeight() - (button_h + margin_between_buttons);
        whats_next_btn->w = m_area.getWidth() - 30;
        whats_next_btn->h = button_h;
        whats_next_btn->setParent(m_irrlicht_window);
        
        whats_next_btn->m_text = _("Back to the main menu");
        whats_next_btn->m_properties[PROP_ID] = "backtomenu";
        
        m_children.push_back(whats_next_btn);
        whats_next_btn->add();
        
        whats_next_btn->setFocusForPlayer( GUI_PLAYER_ID );
    }
    else if (race_manager->getMajorMode() == RaceManager::MAJOR_MODE_GRAND_PRIX)
    {
        ButtonWidget* whats_next_btn = new ButtonWidget();
        whats_next_btn->x = 15;
        whats_next_btn->y = m_area.getHeight() - (button_h + margin_between_buttons)*2;
        whats_next_btn->w = m_area.getWidth() - 30;
        whats_next_btn->h = button_h;
        whats_next_btn->setParent(m_irrlicht_window);
        
        whats_next_btn->m_text = _("Continue Grand Prix");
        whats_next_btn->m_properties[PROP_ID] = "continuegp";
        
        m_children.push_back(whats_next_btn);
        whats_next_btn->add();
        
        whats_next_btn->setFocusForPlayer( GUI_PLAYER_ID );
        
        
        ButtonWidget* abort_gp = new ButtonWidget();
        abort_gp->m_properties[PROP_ID] = "backtomenu";
        abort_gp->x = 15;
        abort_gp->y = m_area.getHeight() - (button_h + margin_between_buttons);
        abort_gp->w = m_area.getWidth() - 30;
        abort_gp->h = button_h;
        abort_gp->m_text = _("Abort Grand Prix");
        abort_gp->setParent(m_irrlicht_window);
        m_children.push_back(abort_gp);
        abort_gp->add();
        

    }
    else
    {
        assert(false);
    }

}

// ------------------------------------------------------------------------------------------------------

void RaceOverDialog::onEnterPressedInternal()
{
}

// ------------------------------------------------------------------------------------------------------

GUIEngine::EventPropagation RaceOverDialog::processEvent(const std::string& eventSource)
{
    if (eventSource == "raceagainbtn")
    {
        ModalDialog::dismiss();
        network_manager->setState(NetworkManager::NS_MAIN_MENU);
        World::getWorld()->unpause();
        race_manager->rerunRace();
        return GUIEngine::EVENT_BLOCK;
    }
    else if (eventSource == "newracebtn")
    {
        ModalDialog::dismiss();
        World::getWorld()->unpause();
        race_manager->exitRace();
        StateManager::get()->resetAndGoToScreen(MainMenuScreen::getInstance());
        StateManager::get()->pushScreen(KartSelectionScreen::getInstance());
        return GUIEngine::EVENT_BLOCK;
    }
    else if (eventSource == "backtomenu")
    {
        ModalDialog::dismiss();
        World::getWorld()->unpause();
        race_manager->exitRace();
        StateManager::get()->resetAndGoToScreen(MainMenuScreen::getInstance());
        return GUIEngine::EVENT_BLOCK;
    }
    else if (eventSource == "continuegp")
    {
        ModalDialog::dismiss();
        World::getWorld()->unpause();
        race_manager->next();
        return GUIEngine::EVENT_BLOCK;
    }
    return GUIEngine::EVENT_LET;
}

//-----------------------------------------------------------------------------

void RaceOverDialog::escapePressed()
{
    if (race_manager->getMajorMode() == RaceManager::MAJOR_MODE_GRAND_PRIX)
    {
        std::string what = "continuegp";
        processEvent(what);
    }
    else if (race_manager->getMajorMode() == RaceManager::MAJOR_MODE_SINGLE)
    {
        std::string what = "backtomenu";
        processEvent(what);
    }
    else
    {
        assert(false);
    }
}

//-----------------------------------------------------------------------------

void RaceOverDialog::onUpdate(float dt)
{
    // Draw battle report (if relevant)
    if (race_manager->getMinorMode() == RaceManager::MINOR_MODE_3_STRIKES)
    {
        // only draw if we have enough space (FIXME: find better way?)
        if (m_buttons_y_from - m_rankings_y_bottom >= 100)
        {
            renderThreeStrikesGraph(m_area.UpperLeftCorner.X + 15, m_rankings_y_bottom + 30,
                                    m_area.getWidth() - 30,
                                    m_buttons_y_from - m_rankings_y_bottom - 45);
        }
        
    }
}

//-----------------------------------------------------------------------------

void RaceOverDialog::renderThreeStrikesGraph(const int x, const int y, const int w, const int h)
{
    assert(h > 0);
    assert(w > 0);
    assert(x >= 0);
    assert(y >= 0);
    
    ThreeStrikesBattle* world = dynamic_cast<ThreeStrikesBattle*>(World::getWorld());
    assert(world != NULL);
    
    const unsigned int evt_count = world->m_battle_events.size();
    if (evt_count < 2) return;
    
    SColor black(255,0,0,0);
    irr_driver->getVideoDriver()->draw2DLine( core::position2d<s32>(x, y),
                                              core::position2d<s32>(x, y + h), black);
    irr_driver->getVideoDriver()->draw2DLine( core::position2d<s32>(x, y + h),
                                              core::position2d<s32>(x + w, y +  h), black);
    
    float lastEventTime = 0.0f;
    int max_life_count = 0;
    
    const unsigned int kart_count = world->m_battle_events[0].m_kart_info.size();
    
    for (unsigned int n=0; n<evt_count; ++n)
    {
        const float time = world->m_battle_events[n].m_time;
        if (time > lastEventTime) lastEventTime = time;
        
        assert(world->m_battle_events[n].m_kart_info.size() == kart_count);

        for (unsigned int k=0; k<kart_count; k++)
        {
            if (world->m_battle_events[n].m_kart_info[k].m_lives > max_life_count)
            {
                max_life_count = world->m_battle_events[n].m_kart_info[k].m_lives;
            }
        }
    }
    
    assert(lastEventTime > 0.0f);
    assert(max_life_count > 0);
    
    for (unsigned int n=1; n<evt_count; ++n)
    {
        const float previous_time = world->m_battle_events[n-1].m_time;
        const float time = world->m_battle_events[n].m_time;
        
        //printf("At time %f :\n", time);
        
        const int previous_event_x  = (int)(x + w*previous_time/lastEventTime);
        const int event_x           = (int)(x + w*time/lastEventTime);
                
        assert(world->m_battle_events[n].m_kart_info.size() == kart_count);

        for (unsigned int k=0; k<kart_count; k++)
        {
            const int kart_lives          = world->m_battle_events[n].m_kart_info[k].m_lives;
            const int kart_previous_lives = world->m_battle_events[n-1].m_kart_info[k].m_lives;

            //printf("    kart %s had %i lives\n", world->getKart(k)->getIdent().c_str(),
            //       world->m_battle_events[n].m_kart_info[k].m_lives);
            
            const int event_y          = y + h - (int)(h * (float)kart_lives / (float)max_life_count);
            const int previous_event_y = y + h - (int)(h * (float)kart_previous_lives / (float)max_life_count);

            const video::SColor& color = world->getKart(k)->getKartProperties()->getColor();
            
            irr_driver->getVideoDriver()->draw2DLine( core::position2d<s32>(previous_event_x, previous_event_y),
                                                      core::position2d<s32>(event_x, previous_event_y), color);
            irr_driver->getVideoDriver()->draw2DLine( core::position2d<s32>(event_x, previous_event_y),
                                                      core::position2d<s32>(event_x, event_y), color);

        }
    }
    
}
