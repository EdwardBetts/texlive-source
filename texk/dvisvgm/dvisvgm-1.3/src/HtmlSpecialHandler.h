/*************************************************************************
** HtmlSpecialHandler.h                                                 **
**                                                                      **
** This file is part of dvisvgm -- the DVI to SVG converter             **
** Copyright (C) 2005-2013 Martin Gieseking <martin.gieseking@uos.de>   **
**                                                                      **
** This program is free software; you can redistribute it and/or        **
** modify it under the terms of the GNU General Public License as       **
** published by the Free Software Foundation; either version 3 of       **
** the License, or (at your option) any later version.                  **
**                                                                      **
** This program is distributed in the hope that it will be useful, but  **
** WITHOUT ANY WARRANTY; without even the implied warranty of           **
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the         **
** GNU General Public License for more details.                         **
**                                                                      **
** You should have received a copy of the GNU General Public License    **
** along with this program; if not, see <http://www.gnu.org/licenses/>. **
*************************************************************************/

#ifndef HTMLSPECIALHANDLER_H
#define HTMLSPECIALHANDLER_H

#include <map>
#include <string>
#include "SpecialHandler.h"

struct SpecialActions;

class HtmlSpecialHandler : public SpecialHandler, public DVIEndPageListener, public DVIPositionListener
{
	struct NamedAnchor {
		NamedAnchor () : id(0), pos(0) {}
		NamedAnchor (int i, double p) : id(i), pos(p) {}
		int id;      ///< unique numerical ID (< 0 if anchor is unreferenced)
		double pos;  ///< vertical position of named anchor (in TeX point units)
	};

	enum AnchorType {AT_NONE, AT_HREF, AT_NAME};
	typedef std::map<std::string, NamedAnchor> NamedAnchors;

   public:
		HtmlSpecialHandler () : _actions(0), _anchorType(AT_NONE), _anchorYPos(0) {}
		bool process (const char *prefix, std::istream &in, SpecialActions *actions);
		const char* name () const  {return "html";}
		const char* info () const  {return "hyperref specials";}
		const char** prefixes () const;

		enum LinkMarker {LM_NONE, LM_LINE, LM_BOX};
		static LinkMarker LINK_MARKER;  ///< selects how linked areas are marked

	protected:
		void hrefAnchor (std::string uri);
		void nameAnchor (const std::string &name);
		void dviEndPage (unsigned pageno);
		void dviMovedTo (double x, double y);
		void closeAnchor ();
		void markLinkedBox ();

	private:
		SpecialActions *_actions;
		AnchorType _anchorType;     ///< type of active anchor
		double _anchorYPos;         ///< current vertical position inside linked area
		std::string _anchorName;    ///< name of currently active named anchor
		std::string _base;          ///< base URL that is prepended to all relative targets
		NamedAnchors _namedAnchors; ///< information about all named anchors processed
};

#endif