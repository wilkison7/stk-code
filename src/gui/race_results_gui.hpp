//  $Id$
//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2006 Eduardo Hernandez Munoz
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
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

#ifndef HEADER_RACERESULTSGUI_H
#define HEADER_RACERESULTSGUI_H

#include "base_gui.hpp"

/** GUI that shows the RaceResults, times and such */
class RaceResultsGUI : public BaseGUI
{
private:
    char* m_score;
    char* m_highscores;

public:
    RaceResultsGUI();
    ~RaceResultsGUI();

    void inputKeyboard(int, int);

    void select();
};

#endif

/* EOF */
