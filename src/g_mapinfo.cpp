/*
** g_mapinfo.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/
 
#include "gamemap.h"
#include "g_mapinfo.h"
#include "tarray.h"
#include "scanner.h"
#include "w_wad.h"

static LevelInfo defaultMap;
static TArray<LevelInfo> levelInfos;

LevelInfo::LevelInfo() : UseMapInfoName(false)
{
	DefaultTexture[0].SetInvalid();
	DefaultTexture[1].SetInvalid();
}

LevelInfo &LevelInfo::Find(const char* level)
{
	for(unsigned int i = 0;i < levelInfos.Size();++i)
	{
		if(levelInfos[i].Name.CompareNoCase(level) == 0)
			return levelInfos[i];
	}
	return defaultMap;
}

void ParseMap(Scanner &sc, LevelInfo &mapInfo, bool parseHeader=true)
{
	if(parseHeader)
	{
		if(sc.CheckToken(TK_Identifier))
		{
			if(sc->str.CompareNoCase("lookup") != 0)
				sc.ScriptMessage(Scanner::ERROR, "Expected lookup keyword but got '%s' instead.", sc->str.GetChars());
		}
		if(sc.CheckToken(TK_StringConst))
		{
			mapInfo.UseMapInfoName = true;
			mapInfo.Name = sc->str;
		}
	}

	sc.MustGetToken('{');
	while(!sc.CheckToken('}'))
	{
		sc.MustGetToken(TK_Identifier);
		FString key = sc->str;

		sc.MustGetToken('=');

		if(key.CompareNoCase("next") == 0)
		{
			sc.MustGetToken(TK_StringConst);
			strncpy(mapInfo.NextMap, sc->str.GetChars(), 8);
			mapInfo.NextMap[8] = 0;
		}
		else if(key.CompareNoCase("secretnext") == 0)
		{
			sc.MustGetToken(TK_StringConst);
			strncpy(mapInfo.NextSecret, sc->str.GetChars(), 8);
			mapInfo.NextSecret[8] = 0;
		}
		else if(key.CompareNoCase("defaultfloor") == 0)
		{
			sc.MustGetToken(TK_StringConst);
			mapInfo.DefaultTexture[MapSector::Floor] = TexMan.GetTexture(sc->str, FTexture::TEX_Flat);
		}
		else if(key.CompareNoCase("defaultceiling") == 0)
		{
			sc.MustGetToken(TK_StringConst);
			mapInfo.DefaultTexture[MapSector::Ceiling] = TexMan.GetTexture(sc->str, FTexture::TEX_Flat);
		}
		else
		{
			sc.ScriptMessage(Scanner::WARNING, "Unknown map property '%s'!", key.GetChars());
			do
			{
				sc.GetNextToken();
			}
			while(sc.CheckToken(','));
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void ParseMapInfoLump(int lump)
{
	FMemLump data = Wads.ReadLump(lump);
	Scanner sc((const char*)data.GetMem(), data.GetSize());
	sc.SetScriptIdentifier(Wads.GetLumpFullName(lump));

	while(sc.TokensLeft())
	{
		sc.MustGetToken(TK_Identifier);
		if(sc->str.CompareNoCase("defaultmap") == 0)
		{
			defaultMap = LevelInfo();
			ParseMap(sc, defaultMap, false);
		}
		else if(sc->str.CompareNoCase("adddefaultmap") == 0)
		{
			ParseMap(sc, defaultMap, false);
		}
		else if(sc->str.CompareNoCase("map") == 0)
		{
			LevelInfo newMap = defaultMap;
			ParseMap(sc, newMap);
			levelInfos.Push(newMap);
		}
	}
}

void G_ParseMapInfo()
{
	int lastlump = 0;
	int lump;

	if((lump = Wads.GetNumForFullName("mapinfo/wolf3d.txt")) != -1)
		ParseMapInfoLump(lump);

	while((lump = Wads.FindLump("ZMAPINFO", &lastlump)) != -1)
		ParseMapInfoLump(lump);
}
