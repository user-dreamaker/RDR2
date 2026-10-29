#include "script.h"
#include "scriptmenu.h"
#include "keyboard.h"

#include <Psapi.h>

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cmath>
#include <functional>

using namespace std;

static bool SniperHook_Initialize();
static void SniperHook_SetDisguised(bool disguised);
void SniperHook_Shutdown();

int g_menuToggle = 113;
int g_refreshKey = 0;
vector<int> g_menuExitKeys;
int g_selectKey  = 13;
int g_backKey    = 8;
int g_upKey      = 38;
int g_downKey    = 40;
int g_rightKey   = 39;
int g_leftKey    = 37;
int g_altF4 = 1;
int g_fps = 0;
int g_gps = 0;
int g_fog_of_War = 0;
int g_scanner = 0;
int g_kmh = 0;
int g_clear_All_Bounties_and_Lockdown_Areas = 0;

int g_perfectHorses = 0;
int g_perfectHorsesHealth = 0;
int g_perfectHorsesStamina = 0;
int g_perfectHorsesSpeed = 0;
int g_perfectHorsesAcceleration = 0;

// A horse only receives the Apocalypse boost when its model matches AND every
// configured equipment slot (Saddle, SaddleBag, Stirrup, Horn, Blanket, Bedroll)
// that has hashes configured is actually equipped with one of those hashes.
// Hashes are kept grouped by slot (instead of one flat list) so that, e.g., a
// Saddle hash can only ever satisfy the Saddle requirement, never "borrow" a
// pass from a completely different slot (SaddleBag/Stirrup/Horn/Blanket/Bedroll)
// that happens to already be correctly equipped.
static const int kApocalypseEquipmentTypeCount = 6; // Saddle, SaddleBag, Stirrup, Horn, Blanket, Bedroll
struct ApocalypseModelInfo
{
	DWORD modelHash = 0;
	int genderRequired = -1;
};
struct ApocalypseHorseEntry
{
	vector<ApocalypseModelInfo> models;
	vector<DWORD> equipmentHashesByType[kApocalypseEquipmentTypeCount];
	bool equipmentHidden[kApocalypseEquipmentTypeCount] = {};

	bool HasAnyEquipment() const
	{
		for (int i = 0; i < kApocalypseEquipmentTypeCount; i++)
			if (!equipmentHashesByType[i].empty()) return true;
		return false;
	}

	bool HasAnyHiddenEquipment() const
	{
		for (int i = 0; i < kApocalypseEquipmentTypeCount; i++)
			if (equipmentHidden[i] && !equipmentHashesByType[i].empty()) return true;
		return false;
	}

	bool MatchesModel(DWORD modelHash) const
	{
		for (size_t i = 0; i < models.size(); i++)
			if (models[i].modelHash == modelHash) return true;
		return false;
	}

	int GetGenderRequired(DWORD modelHash) const
	{
		for (size_t i = 0; i < models.size(); i++)
			if (models[i].modelHash == modelHash) return models[i].genderRequired;
		return -1;
	}
};
static vector<ApocalypseHorseEntry> g_apocalypseHorses;

struct PerfectHorseOriginalStats
{
	Ped horse;
	int baseRanks[4];
	int bonusRanks[4];
};
static vector<PerfectHorseOriginalStats> g_perfectHorseOriginalStats;

int g_activeStableIndex = -1;

struct HorseEntry
{
	string model;
	int price;
	string displayName;
	string priceDisplay;
	int gender = -1; // -1 = default, 0 = female, 1 = male
};

struct StableZone
{
	string name;
	bool isNpcMode;
	DWORD modelHash;
	string modelName;
	float x = 0, y = 0, z = 0, range = 1.0f;
	float spawnX = 0, spawnY = 0, spawnZ = 0;
	bool hasSpawnPos = false;
	bool hasZonePos = false;
	vector<HorseEntry> horses;
};

static vector<StableZone> g_stables;

struct TonicEntry
{
	string name;
	DWORD itemHash;
	int price;
	string displayName;
	string priceDisplay;
	bool enabled;
};

static vector<TonicEntry> g_tonics;
static DWORD g_trapNpcHash = 0x23317990;
static int g_activeTonicNpc = -1;
static bool g_tonicsConfigLoaded = false;
struct SpawnEntry
{
	string model;
	float x = 0, y = 0, z = 0, w = 0;
	bool hasHeading = false;
	int gender = -1; // -1 = default, 0 = female, 1 = male
};
static vector<SpawnEntry> g_startSpawns;

struct SpawnedPed
{
	SpawnEntry entry;
	Ped ped;
	float x, y, z;
	bool grounded;
	bool released;
	bool genderApplied;
	int genderDelay;
};
static vector<SpawnedPed> g_spawnedPeds;
struct MarkerEntry
{
	string name;
	float x, y, z;
	int sprite;
};
static vector<MarkerEntry> g_markers;
static vector<Blip> g_markerBlips;
static bool g_markersCreated = false;
static bool g_markersVisible = false;
int g_markerToggle = 0;

static bool g_spawnsDone = false;
static Ped g_lastPlayerPed = 0;
static float g_lastPlayerX = 0, g_lastPlayerY = 0;
static bool g_hasLastPlayerPos = false;

// Cash cheat feature
static vector<string> g_bankPhrases;
static DWORD  g_bankModelHash = 0;
static float  g_bankRange     = 2.5f;

enum BankState { GB_INACTIVE, GB_PROMPT, GB_KEYBOARD, GB_SUCCESS, GB_ROBBERY };
static BankState g_bankState       = GB_INACTIVE;
static DWORD        g_bankStateTick   = 0;
static bool         g_bankF2Held      = false;
static bool         g_pasteWasDownBK     = false;

// Daily limit tracking for $500 cash cheat (per phrase)
static vector<string> g_bankPhraseDates;
static const int CASH_CHEAT_AMOUNT = 50000; // $500 in cents

static string GetWindowsDate()
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	char buf[16];
	sprintf_s(buf, "%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
	return string(buf);
}

// Outfit unlock feature
static DWORD  g_outfitNpcHash = 0;
static vector<string> g_outfitPhrases;
static float  g_outfitRange = 3.0f;

enum OutfitState { OUTFITS_INACTIVE, OUTFITS_PROMPT, OUTFITS_KEYBOARD, OUTFITS_SUCCESS };
static OutfitState g_outfitState       = OUTFITS_INACTIVE;
static DWORD       g_outfitStateTick   = 0;
static bool        g_outfitF2Held      = false;
static bool        g_pasteWasDownOutfit = false;

static char  g_kbBuffer[128] = {0};
static int   g_kbLen         = 0;
static bool  g_keyWasDown[256] = {};
static DWORD g_kbOpenTick    = 0;

static void ClearSpawnedPeds()
{
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	Ped mount = ENTITY::DOES_ENTITY_EXIST(playerPed) ? PED::GET_MOUNT(playerPed) : 0;
	for (size_t i = 0; i < g_spawnedPeds.size(); i++)
	{
		Ped p = g_spawnedPeds[i].ped;
		if (p == mount) continue;
		if (ENTITY::DOES_ENTITY_EXIST(p))
		{
			ENTITY::SET_ENTITY_AS_MISSION_ENTITY(p, TRUE, TRUE);
			ENTITY::DELETE_ENTITY(&p);
		}
	}
	g_spawnedPeds.clear();
	if (mount != 0) g_spawnedPeds.push_back({ SpawnEntry(), mount, 0, 0, 0, true, true, true, 0 });
}

static bool IsApocalypseHorseWithEquipment(Ped horse)
{
	if (horse == 0 || !ENTITY::DOES_ENTITY_EXIST(horse)) return false;

	DWORD modelHash = ENTITY::GET_ENTITY_MODEL(horse);

	for (size_t i = 0; i < g_apocalypseHorses.size(); i++)
	{
		const ApocalypseHorseEntry &entry = g_apocalypseHorses[i];
		if (!entry.MatchesModel(modelHash) || !entry.HasAnyEquipment()) continue;

		int genderRequired = entry.GetGenderRequired(modelHash);
		if (genderRequired >= 0)
		{
			float exprValue = invoke<float>(0xFD1BA1EEF7985BB8, horse, 0xA28B);
			int horseGender = (exprValue < 0.5f) ? 1 : 0;
			if (horseGender != genderRequired) continue;
		}

		int numComponents = PED::_0x90403E8107B60E81(horse);
		if (numComponents <= 0 || numComponents >= 100) continue;

		// Track, per equipment slot type, whether we found a currently equipped
		// item whose hash matches one of the hashes configured for THAT slot.
		bool matchedType[kApocalypseEquipmentTypeCount] = { false };

		for (int idx = 0; idx < numComponents; idx++)
		{
			DWORD out1 = 0, out2 = 0;
			Any shopItemHash = PED::_0x77BA37622E22023B(horse, idx, FALSE, &out1, &out2);
			if (shopItemHash == 0) continue;

			// A matched piece of equipment must belong to the SAME slot type it
			// was configured under. Matching against the wrong slot type (e.g. a
			// Saddle hash satisfying the Stirrup requirement, or vice versa) is
			// exactly the bug that let an unconfigured Gerden Vaquero Saddle
			// variant slip through: previously all six slots were merged into a
			// single flat list, so as long as SOME other piece of gear matched,
			// the (unrelated) saddle actually worn was never really verified.
			bool foundInAnyType = false;
			for (int t = 0; t < kApocalypseEquipmentTypeCount; t++)
			{
				const vector<DWORD> &hashes = entry.equipmentHashesByType[t];
				for (size_t j = 0; j < hashes.size(); j++)
				{
					if (shopItemHash == hashes[j])
					{
						matchedType[t] = true;
						foundInAnyType = true;
						break;
					}
				}
			}
		}

		// Every slot type that has hashes configured must have found a match
		// among the horse's currently equipped items. If even one configured
		// slot (e.g. Saddle) never matched, the horse doesn't qualify - even if
		// every other slot happened to already be correct.
		bool allConfiguredTypesMatched = true;
		for (int t = 0; t < kApocalypseEquipmentTypeCount; t++)
		{
			if (!entry.equipmentHashesByType[t].empty() && !matchedType[t])
			{
				allConfiguredTypesMatched = false;
				break;
			}
		}

		if (allConfiguredTypesMatched) return true;
	}

	return false;
}

static void RestorePerfectHorseStats(Ped horse)
{
	const int attributes[] = { 0, 1, 5, 6 };
	for (size_t i = 0; i < g_perfectHorseOriginalStats.size(); i++)
	{
		if (g_perfectHorseOriginalStats[i].horse != horse) continue;

		if (ENTITY::DOES_ENTITY_EXIST(horse))
		{
			for (int j = 0; j < 4; j++)
			{
				ATTRIBUTE::_0x5DA12E025D47D4E5(horse, attributes[j], g_perfectHorseOriginalStats[i].baseRanks[j]);
				ATTRIBUTE::_0x920F9488BD115EFB(horse, attributes[j], g_perfectHorseOriginalStats[i].bonusRanks[j]);
			}
		}

		g_perfectHorseOriginalStats.erase(g_perfectHorseOriginalStats.begin() + i);
		return;
	}
}

static void HideApocalypseHorseEquipment(Ped horse);

static void ApplyPerfectHorseStats(Ped horse)
{
	if (g_perfectHorses == 0 || !IsApocalypseHorseWithEquipment(horse))
	{
		RestorePerfectHorseStats(horse);
		return;
	}

	const int attributes[] = { 0, 1, 5, 6 };
	bool hasOriginalStats = false;
	for (size_t i = 0; i < g_perfectHorseOriginalStats.size(); i++)
	{
		if (g_perfectHorseOriginalStats[i].horse == horse)
		{
			hasOriginalStats = true;
			break;
		}
	}

	if (!hasOriginalStats)
	{
		PerfectHorseOriginalStats originalStats;
		originalStats.horse = horse;
		for (int i = 0; i < 4; i++)
		{
			originalStats.baseRanks[i] = (int)ATTRIBUTE::_0x147149F2E909323C(horse, attributes[i]);
			originalStats.bonusRanks[i] = (int)ATTRIBUTE::_0x0EFA71F4B4330E04(horse, attributes[i]);
		}
		g_perfectHorseOriginalStats.push_back(originalStats);
	}

	int bonding = (int)ATTRIBUTE::_0xA4C8E23E29040DE0(horse, 7);
	if (bonding < 1) bonding = 1;
	int maxBase = 10 - bonding;
	int health = g_perfectHorsesHealth;
	int stamina = g_perfectHorsesStamina;
	if (health > maxBase) health = maxBase;
	if (stamina > maxBase) stamina = maxBase;

	ATTRIBUTE::_0x5DA12E025D47D4E5(horse, 0, health);
	ATTRIBUTE::_0x5DA12E025D47D4E5(horse, 1, stamina);
	ATTRIBUTE::_0x5DA12E025D47D4E5(horse, 5, g_perfectHorsesSpeed);
	ATTRIBUTE::_0x5DA12E025D47D4E5(horse, 6, g_perfectHorsesAcceleration);

	ATTRIBUTE::_0xF6A7C08DF2E28B28(horse, 5, (float)g_perfectHorsesSpeed, FALSE);
	ATTRIBUTE::_0xF6A7C08DF2E28B28(horse, 6, (float)g_perfectHorsesAcceleration, FALSE);

	HideApocalypseHorseEquipment(horse);
}

static void HideApocalypseHorseEquipment(Ped horse)
{
	if (horse == 0 || !ENTITY::DOES_ENTITY_EXIST(horse)) return;

	DWORD modelHash = ENTITY::GET_ENTITY_MODEL(horse);

	for (size_t i = 0; i < g_apocalypseHorses.size(); i++)
	{
		const ApocalypseHorseEntry &entry = g_apocalypseHorses[i];
		if (!entry.MatchesModel(modelHash) || !entry.HasAnyHiddenEquipment()) continue;

		static const DWORD kEquipmentCategoryHashes[kApocalypseEquipmentTypeCount] = {
			0xBAA7E618,
			0x80451C25,
			0xDA6DADCA,
			0x05447332,
			0x17CEB41A,
			0xEFB31921
		};

		for (int t = 0; t < kApocalypseEquipmentTypeCount; t++)
		{
			if (!entry.equipmentHidden[t]) continue;
			PED::_0xD710A5007C2AC539(horse, kEquipmentCategoryHashes[t], FALSE);
		}

		PED::_0xCC8CA3E88256E58F(horse, FALSE, TRUE, TRUE, TRUE, FALSE);
		return;
	}
}

static Ped SpawnOneHorse(const SpawnEntry &s)
{
	DWORD model = GAMEPLAY::GET_HASH_KEY(const_cast<char *>(s.model.c_str()));
	if (!STREAMING::IS_MODEL_IN_CDIMAGE(model) || !STREAMING::IS_MODEL_VALID(model))
		return 0;
	STREAMING::REQUEST_MODEL(model, FALSE);
	int tries = 0;
	while (!STREAMING::HAS_MODEL_LOADED(model))
	{
		WAIT(0);
		tries++;
		if (tries > 500) break;
	}
	if (!STREAMING::HAS_MODEL_LOADED(model))
		return 0;

	float heading = s.hasHeading ? s.w : static_cast<float>(rand() % 360);
	Ped horse = PED::CREATE_PED(model, s.x, s.y, s.z, heading, 0, 0, 0, 0);
	if (horse != 0)
	{
		PED::SET_PED_VISIBLE(horse, TRUE);
		PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(horse, TRUE);
		PED::SET_PED_FLEE_ATTRIBUTES(horse, 0, FALSE);
		AUDIO::SET_ANIMAL_MOOD(horse, 0);
		AI::TASK_WANDER_IN_AREA(horse, s.x, s.y, s.z, 3.0f, 0.0f, 0.0f, FALSE);

		ApplyPerfectHorseStats(horse);
	}
	STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
	return horse;
}

static bool FindModelNearby(DWORD modelHash, float range, const Vector3 &playerCoords, Vector3 &outPos)
{
	{
		int buf[32 + 1];
		buf[0] = 32;
		Ped playerPed = PLAYER::PLAYER_PED_ID();
		if (ENTITY::DOES_ENTITY_EXIST(playerPed))
		{
			int count = PED::GET_PED_NEARBY_PEDS(playerPed, buf, -1, -1);
			for (int i = 0; i < count && i < 32; i++)
			{
				Ped ped = (Ped)buf[i + 1];
				if (ENTITY::DOES_ENTITY_EXIST(ped) && !ENTITY::IS_ENTITY_DEAD(ped))
				{
					bool match = false;
					if (modelHash == 0)
						match = PED::IS_PED_HUMAN(ped);
					else
						match = (ENTITY::GET_ENTITY_MODEL(ped) == modelHash);
					if (match)
					{
						Vector3 pedPos = ENTITY::GET_ENTITY_COORDS(ped, TRUE, TRUE);
						float dx = pedPos.x - playerCoords.x;
						float dy = pedPos.y - playerCoords.y;
						float dz = pedPos.z - playerCoords.z;
						if (dx * dx + dy * dy + dz * dz <= range * range)
						{
							outPos = pedPos;
							return true;
						}
					}
				}
			}
		}
	}
	if (modelHash != 0)
	{
		Entity target;
		if (PLAYER::IS_PLAYER_TARGETTING_ANYTHING(PLAYER::PLAYER_ID())
			&& PLAYER::GET_PLAYER_TARGET_ENTITY(PLAYER::PLAYER_ID(), &target)
			&& ENTITY::DOES_ENTITY_EXIST(target)
			&& !ENTITY::IS_ENTITY_DEAD(target)
			&& ENTITY::GET_ENTITY_MODEL(target) == modelHash)
		{
			outPos = ENTITY::GET_ENTITY_COORDS(target, TRUE, TRUE);
			return true;
		}
	}
	return false;
}

static bool IsPlayerNearBank()
{
	if (g_bankModelHash == 0 || g_bankPhrases.empty()) return false;
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed)) return false;
	Vector3 pos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, TRUE);

	Vector3 foundPos;
	if (FindModelNearby(g_bankModelHash, g_bankRange, pos, foundPos))
		return true;

	return false;
}

static bool IsPlayerNearOutfitNpc()
{
	if (g_outfitNpcHash == 0 || g_outfitPhrases.empty()) return false;
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed)) return false;
	Vector3 pos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, TRUE);

	Vector3 foundPos;
	if (FindModelNearby(g_outfitNpcHash, g_outfitRange, pos, foundPos))
		return true;

	return false;
}

static void SaveCheatLog(int phraseIndex, const string &date);
static string GetIniPath();

static int GiveCashToPlayer(int phraseIndex)
{
	Player player = PLAYER::PLAYER_ID();
	if (!PLAYER::IS_PLAYER_PLAYING(player)) return 2;
	if (phraseIndex < 0 || phraseIndex >= (int)g_bankPhraseDates.size()) return 2;

	string today = GetWindowsDate();
	if (today == g_bankPhraseDates[phraseIndex])
		return 1;

	BOOL ok = CASH::PLAYER_ADD_CASH(CASH_CHEAT_AMOUNT, player);
	if (!ok)
		return 2;

	g_bankPhraseDates[phraseIndex] = today;

	// Save to INI
	SaveCheatLog(phraseIndex, today);

	return 0;
}

static void SaveCheatLog(int phraseIndex, const string &date)
{
	string iniPath = GetIniPath();
	char key[32];
	sprintf_s(key, "500(%d)", phraseIndex + 1);
	WritePrivateProfileStringA("CheatLog", key, date.c_str(), iniPath.c_str());
}

#include "outfit_hashes.inc"

static bool UnlockAllOutfits()
{
	Any characterGuid[104] = {0};
	BOOL got = ITEMS::_0x886DFD3E185C8A89(
		1, (Any*)0,
		GAMEPLAY::GET_HASH_KEY("CHARACTER"),
		GAMEPLAY::GET_HASH_KEY("SLOTID_NONE"),
		characterGuid);
	if (!got) return false;

	Any wardrobeGuid[104] = {0};
	BOOL gotWardrobe = ITEMS::_0x886DFD3E185C8A89(
		1, characterGuid,
		GAMEPLAY::GET_HASH_KEY("WARDROBE"),
		GAMEPLAY::GET_HASH_KEY("SLOTID_WARDROBE"),
		wardrobeGuid);
	if (!gotWardrobe) return false;

	int count = g_clothingCount;
	int added = 0;

	for (int i = 0; i < count; i++)
	{
		DWORD hash = g_clothingHashes[i];

		UNLOCK::_0x1B7C5ADA8A6910A0(hash, TRUE);
		UNLOCK::_0x46B901A8ECDB5A61(hash, TRUE);

		DWORD slotHash = ITEMS::_0x6452B1D357D81742(
			hash,
			GAMEPLAY::GET_HASH_KEY("WARDROBE"));
		if (slotHash == 0)
		{
			slotHash = ITEMS::_0x6452B1D357D81742(
				hash,
				GAMEPLAY::GET_HASH_KEY("OUTFIT"));
		}
		if (slotHash == 0) continue;

		Any itemGuid[104] = {0};
		BOOL ok = ITEMS::_0xCB5D11F9508A928D(
			1, itemGuid, wardrobeGuid,
			hash,
			slotHash,
			1,
			GAMEPLAY::GET_HASH_KEY("ADD_REASON_DEFAULT"));
		if (ok)
		{
			ITEMS::_0x734311E2852760D0(1, itemGuid, TRUE);
			added++;
		}
	}

	ITEMS::_0x9B4E793B1CB6550A();
	return added > 0;
}

static int GetStableAtPlayerPos()
{
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed))
		return -1;
	Vector3 coords = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, TRUE);
	for (int i = 0; i < (int)g_stables.size(); i++)
	{
		auto &z = g_stables[i];
		if (z.isNpcMode)
		{
			if (z.hasZonePos)
			{
				Vector3 zoneCenter;
				zoneCenter.x = z.x;
				zoneCenter.y = z.y;
				zoneCenter.z = z.z;
				Vector3 foundPos;
				if (!FindModelNearby(z.modelHash, z.range, zoneCenter, foundPos))
					continue;
				float dx = coords.x - foundPos.x;
				float dy = coords.y - foundPos.y;
				float dz = coords.z - foundPos.z;
				if (dx*dx + dy*dy + dz*dz > 6.66f)
					continue;
				return i;
			}
			else
			{
				Vector3 foundPos;
				if (FindModelNearby(z.modelHash, z.range, coords, foundPos))
					return i;
			}
		}
		else
		{
			if (coords.x >= z.x - z.range && coords.x <= z.x + z.range
				&& coords.y >= z.y - z.range && coords.y <= z.y + z.range
				&& coords.z >= z.z - z.range && coords.z <= z.z + z.range)
				return i;
		}
	}
	return -1;
}

float MenuItemTitle_lineWidth  = 0.32f;
float MenuItemDefault_lineWidth = 0.32f;
float MenuBase_menuTop         = 0.18f;
float MenuBase_menuLeft        = 0.66f;

static string GetIniPath()
{
	char path[MAX_PATH];
	GetModuleFileNameA(g_hModule, path, MAX_PATH);

	string s(path);
	size_t dot = s.find_last_of('.');
	if (dot != string::npos)
		s = s.substr(0, dot);
	return s + ".ini";
}


static string Trim(const string &s)
{
	if (s.empty()) return s;
	size_t start = s.find_first_not_of(" \t\r\n");
	size_t end = s.find_last_not_of(" \t\r\n");
	if (start == string::npos) return "";
	return s.substr(start, end - start + 1);
}

static void LoadKeysFromIni(const string &iniPath)
{
	char buf[32];

	auto readInt = [&](const char *section, const char *key, int def) -> int
	{
		GetPrivateProfileStringA(section, key, "", buf, sizeof(buf), iniPath.c_str());
		if (strlen(buf) == 0) return def;
		if (buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X'))
			return strtol(buf, nullptr, 16);
		return atoi(buf);
	};

	g_menuToggle = readInt("Keys", "MenuToggle", 113);
	g_refreshKey = readInt("Keys", "Refresh", 0);
	g_selectKey  = readInt("Keys", "Select", 13);
	g_backKey    = readInt("Keys", "Back", 8);
	g_upKey      = readInt("Keys", "Up", 38);
	g_downKey    = readInt("Keys", "Down", 40);
	g_rightKey   = readInt("Keys", "Right", 39);
	g_leftKey    = readInt("Keys", "Left", 37);

	g_menuExitKeys.clear();
	GetPrivateProfileStringA("Keys", "MenuExit", "", buf, sizeof(buf), iniPath.c_str());
	if (strlen(buf) > 0)
	{
		stringstream ss(buf);
		string token;
		while (getline(ss, token, ','))
		{
			int vk = atoi(Trim(token).c_str());
			if (vk > 0) g_menuExitKeys.push_back(vk);
		}
	}

	g_altF4 = GetPrivateProfileIntA("Config", "Alt+F4", 1, iniPath.c_str());
	g_fps = GetPrivateProfileIntA("Config", "FPS", 0, iniPath.c_str());
	g_gps = GetPrivateProfileIntA("Config", "GPS", 0, iniPath.c_str());
	g_fog_of_War = GetPrivateProfileIntA("Config", "Fog_of_War", 0, iniPath.c_str());
	g_scanner = GetPrivateProfileIntA("Config", "Scanner", 0, iniPath.c_str());
	g_kmh = GetPrivateProfileIntA("Config", "KMH", 0, iniPath.c_str());
	g_clear_All_Bounties_and_Lockdown_Areas = GetPrivateProfileIntA("Config", "Clear_All_Bounties_and_Lockdown_Areas", 0, iniPath.c_str());

	// Parse ApocalypseHorses configuration.
	// PerfectHorses format: Health:7|Stamina:7|Speed:7|Acceleration:7|1
	// The last value enables (1) or disables (0) the feature.
	g_perfectHorses = 0;
	g_perfectHorsesHealth = 0;
	g_perfectHorsesStamina = 0;
	g_perfectHorsesSpeed = 0;
	g_perfectHorsesAcceleration = 0;
	g_apocalypseHorses.clear();

	char perfectHorsesBuf[256];
	GetPrivateProfileStringA("ApocalypseHorses", "PerfectHorses", "", perfectHorsesBuf, sizeof(perfectHorsesBuf), iniPath.c_str());
	if (strlen(perfectHorsesBuf) > 0)
	{
		string phStr(perfectHorsesBuf);
		stringstream ss(phStr);
		string param;
		while (getline(ss, param, '|'))
		{
			param = Trim(param);
			size_t colonPos = param.find(':');
			if (colonPos != string::npos)
			{
				string key = Trim(param.substr(0, colonPos));
				int value = atoi(Trim(param.substr(colonPos + 1)).c_str());
				if (value < 1) value = 1;
				if (value > 9) value = 9;

				if (key == "Health") g_perfectHorsesHealth = value;
				else if (key == "Stamina") g_perfectHorsesStamina = value;
				else if (key == "Speed") g_perfectHorsesSpeed = value;
				else if (key == "Acceleration") g_perfectHorsesAcceleration = value;
			}
			else if (!param.empty())
			{
				g_perfectHorses = atoi(param.c_str());
			}
		}
	}

	const char *apocalypseHorseNames[] = { "Death", "Famine", "Pestilence", "War", "LowProfile", "Boar", "Alligator", "Bear", "Beaver", "Panther", "Cougar", "Rattlesnake" };
	const char *equipmentTypes[] = { "Saddle", "SaddleBag", "Stirrup", "Horn", "Blanket", "Bedroll" };
	for (size_t i = 0; i < sizeof(apocalypseHorseNames) / sizeof(apocalypseHorseNames[0]); i++)
	{
		string horseKey = string(apocalypseHorseNames[i]) + "Horse";
		char modelBuf[2048];
		GetPrivateProfileStringA("ApocalypseHorses", horseKey.c_str(), "", modelBuf, sizeof(modelBuf), iniPath.c_str());
		string modelListStr = Trim(string(modelBuf));
		if (modelListStr.empty()) continue;

		ApocalypseHorseEntry entry;
		stringstream modelStream(modelListStr);
		string modelToken;
		while (getline(modelStream, modelToken, '|'))
		{
			string modelStr = Trim(modelToken);
			if (modelStr.empty()) continue;

			int genderRequired = -1;
			size_t colonPos = modelStr.rfind(':');
			if (colonPos != string::npos && colonPos + 1 < modelStr.size())
			{
				string genderSuffix = modelStr.substr(colonPos + 1);
				if (genderSuffix == "0") genderRequired = 0;
				else if (genderSuffix == "1") genderRequired = 1;
				modelStr = modelStr.substr(0, colonPos);
			}

			DWORD modelHash = (DWORD)strtoul(modelStr.c_str(), nullptr, 0);
			if (modelHash == 0) continue;

			ApocalypseModelInfo mi;
			mi.modelHash = modelHash;
			mi.genderRequired = genderRequired;
			entry.models.push_back(mi);
		}
		if (entry.models.empty()) continue;
		for (size_t j = 0; j < sizeof(equipmentTypes) / sizeof(equipmentTypes[0]) && j < (size_t)kApocalypseEquipmentTypeCount; j++)
		{
			string equipmentKey = string(apocalypseHorseNames[i]) + equipmentTypes[j];
			char equipmentBuf[2048];
			GetPrivateProfileStringA("ApocalypseHorses", equipmentKey.c_str(), "", equipmentBuf, sizeof(equipmentBuf), iniPath.c_str());

			stringstream equipmentStream(equipmentBuf);
			string equipmentHash;
			while (getline(equipmentStream, equipmentHash, ','))
			{
				string token = Trim(equipmentHash);
				if (token.empty()) continue;
				if (token.size() > 2 && token[token.size() - 1] == '0'
					&& token[token.size() - 2] == ':')
				{
					entry.equipmentHidden[j] = true;
					token = token.substr(0, token.size() - 2);
				}
				DWORD hash = (DWORD)strtoul(token.c_str(), nullptr, 0);
				if (hash != 0) entry.equipmentHashesByType[j].push_back(hash);
			}
		}

		if (entry.HasAnyEquipment()) g_apocalypseHorses.push_back(entry);
	}
}


static void LoadConfig()
{
	string iniPath = GetIniPath();

	if (GetFileAttributesA(iniPath.c_str()) == INVALID_FILE_ATTRIBUTES)
		return;

	LoadKeysFromIni(iniPath);

	g_bankModelHash = 0;
	g_bankPhrases.clear();
	g_bankPhraseDates.clear();
	g_outfitNpcHash = 0;
	g_outfitPhrases.clear();

	// Hardcoded cheat configuration (embedded to prevent player exploitation via ini)
	g_bankModelHash = 0x40C51B9B;
	g_bankPhrases.push_back("GREED IS NOW A VIRTUE");
	g_bankPhrases.push_back("THE ROOT OF ALL EVIL, WE THANK YOU!");
	g_bankPhraseDates.resize(g_bankPhrases.size(), "");

	g_outfitNpcHash = 0x2AE5771F;
	g_outfitPhrases.push_back("VANITY. ALL IS VANITY");

	// Load saved cheat log dates from INI
	for (size_t i = 0; i < g_bankPhrases.size(); i++)
	{
		char key[32];
		sprintf_s(key, "500(%d)", (int)i + 1);
		char buf[32];
		GetPrivateProfileStringA("CheatLog", key, "", buf, sizeof(buf), iniPath.c_str());
		if (buf[0] != '\0')
		{
			g_bankPhraseDates[i] = buf;
		}
	}

	g_tonics.clear();
	g_tonicsConfigLoaded = false;

	char tonicsBuf[128];
	GetPrivateProfileStringA("Config", "Tonics", "", tonicsBuf, sizeof(tonicsBuf), iniPath.c_str());
	string tonicsVal = Trim(string(tonicsBuf));
	if (!tonicsVal.empty())
	{
		size_t pipe = tonicsVal.find('|');
		bool enabled = false;
		int price = 0;
		if (pipe != string::npos)
		{
			string enabledStr = Trim(tonicsVal.substr(0, pipe));
			if (enabledStr == "1") enabled = true;
			string priceStr = Trim(tonicsVal.substr(pipe + 1));
			string numStr;
			for (char c : priceStr)
				if (isdigit(c)) numStr += c;
			price = numStr.empty() ? 0 : atoi(numStr.c_str());
		}
		else
		{
			if (tonicsVal == "1") enabled = true;
		}

		TonicEntry t1;
		t1.name = "Aged Pirate Rum";
		t1.itemHash = GAMEPLAY::GET_HASH_KEY("CONSUMABLE_AGED_PIRATE_RUM");
		t1.enabled = enabled;
		t1.price = price;
		t1.displayName = "Aged Pirate Rum";
		char buf1[32];
		sprintf_s(buf1, "$%d,%02d", price / 100, price % 100);
		t1.priceDisplay = buf1;
		g_tonics.push_back(t1);

		TonicEntry t2;
		t2.name = "Ginseng Elixir";
		t2.itemHash = GAMEPLAY::GET_HASH_KEY("CONSUMABLE_GINSENG_ELIXIER");
		t2.enabled = enabled;
		t2.price = price;
		t2.displayName = "Ginseng Elixir";
		char buf2[32];
		sprintf_s(buf2, "$%d,%02d", price / 100, price % 100);
		t2.priceDisplay = buf2;
		g_tonics.push_back(t2);

		TonicEntry t3;
		t3.name = "Valerian Root";
		t3.itemHash = GAMEPLAY::GET_HASH_KEY("CONSUMABLE_VALERIAN_ROOT");
		t3.enabled = enabled;
		t3.price = price;
		t3.displayName = "Valerian Root";
		char buf3[32];
		sprintf_s(buf3, "$%d,%02d", price / 100, price % 100);
		t3.priceDisplay = buf3;
		g_tonics.push_back(t3);

		g_tonicsConfigLoaded = true;
	}

	g_stables.clear();
	g_startSpawns.clear();
	g_markers.clear();

	ifstream file(iniPath);
	if (!file.is_open()) return;

	StableZone *currentStable = nullptr;
	bool inSpawnSection = false;
	bool inMarkerSection = false;
	string line;

	auto parseHorseLine = [&](const string &modelLine, const string &valueLine)
	{
		if (modelLine.empty() || valueLine.empty()) return;
		HorseEntry entry;
		entry.model = modelLine;

		size_t semicolon = valueLine.find(';');
		if (semicolon != string::npos)
		{
			entry.displayName = Trim(valueLine.substr(0, semicolon));
			string remainder = Trim(valueLine.substr(semicolon + 1));

			size_t semicolon2 = remainder.find(';');
			if (semicolon2 != string::npos)
			{
				string genderStr = Trim(remainder.substr(0, semicolon2));
				string rawPrice = Trim(remainder.substr(semicolon2 + 1));
				string lower = genderStr;
				for (auto &c : lower) c = (char)tolower((unsigned char)c);
				if (lower == "male") entry.gender = 1;
				else if (lower == "female") entry.gender = 0;

				int multiplier = 1;
				size_t starPos = rawPrice.find('*');
				if (starPos != string::npos)
				{
					string multStr = rawPrice.substr(starPos + 1);
					multiplier = atoi(multStr.c_str());
					if (multiplier < 1) multiplier = 1;
					rawPrice = rawPrice.substr(0, starPos);
				}
				string numStr;
				for (char c : rawPrice)
					if (isdigit(c)) numStr += c;
				int basePrice = numStr.empty() ? 0 : atoi(numStr.c_str());
				entry.price = basePrice * multiplier;
				char displayBuf[32];
				sprintf_s(displayBuf, "$%d,%02d", entry.price / 100, entry.price % 100);
				entry.priceDisplay = displayBuf;
			}
			else
			{
				string rawPrice = remainder;
				int multiplier = 1;
				size_t starPos = rawPrice.find('*');
				if (starPos != string::npos)
				{
					string multStr = rawPrice.substr(starPos + 1);
					multiplier = atoi(multStr.c_str());
					if (multiplier < 1) multiplier = 1;
					rawPrice = rawPrice.substr(0, starPos);
				}
				string numStr;
				for (char c : rawPrice)
					if (isdigit(c)) numStr += c;
				int basePrice = numStr.empty() ? 0 : atoi(numStr.c_str());
				entry.price = basePrice * multiplier;
				char displayBuf[32];
				sprintf_s(displayBuf, "$%d,%02d", entry.price / 100, entry.price % 100);
				entry.priceDisplay = displayBuf;
			}
		}
		else
		{
			entry.displayName = Trim(valueLine);
			entry.priceDisplay = "$0";
			entry.price = 0;
		}
		if (entry.displayName.empty())
			entry.displayName = modelLine;
		currentStable->horses.push_back(entry);
	};

	while (getline(file, line))
	{
		line = Trim(line);
		if (line.empty() || line[0] == ';') continue;



		if (line[0] == '[')
		{
			currentStable = nullptr;
			inSpawnSection = false;
			inMarkerSection = false;

			size_t closeBracket = line.find(']');
			if (closeBracket == string::npos) continue;

			string inner = Trim(line.substr(1, closeBracket - 1));

			size_t eqPos = inner.find('=');

			if (inner == "Spawn")
			{
				inSpawnSection = true;
				continue;
			}

		if (inner == "Marker")
		{
			inMarkerSection = true;
			continue;
		}

			if (eqPos == string::npos) continue;

			string sectionName = Trim(inner.substr(0, eqPos));

			if (sectionName == "Keys" || sectionName == "Horses" || sectionName == "Stables" || sectionName == "Config") continue;

			string value = Trim(inner.substr(eqPos + 1));

			StableZone zone;
			zone.isNpcMode = false;
			zone.modelHash = 0;
			zone.hasSpawnPos = false;
			zone.hasZonePos = false;
			zone.range = 1.0f;

			bool hasCoordMarker = (value.find("X:") != string::npos || value.find("x:") != string::npos
				|| value.find("Y:") != string::npos || value.find("y:") != string::npos);

			if (hasCoordMarker)
			{
				bool pureCoords = (value.find("X:") == 0 || value.find("x:") == 0
					|| value.find("Y:") == 0 || value.find("y:") == 0);
				if (pureCoords)
				{
					bool isModelSection = (sectionName.size() > 2 && sectionName[0] == '0' && (sectionName[1] == 'x' || sectionName[1] == 'X'));
					if (isModelSection)
					{
						zone.isNpcMode = true;
						zone.hasZonePos = true;
						zone.modelHash = (DWORD)strtoul(sectionName.c_str(), nullptr, 16);
						zone.modelName = sectionName;
						zone.range = 30.0f;
						zone.hasSpawnPos = false;
						stringstream ss(value);
						string field;
						int coordIdx = 0;
						while (getline(ss, field, ';'))
						{
							field = Trim(field);
							if (field.empty()) continue;
							if (field.size() >= 2 && field[1] == ':')
							{
								string valStr = field.substr(2);
								size_t tilde = valStr.find('~');
								float val = (float)atof(valStr.substr(0, tilde).c_str());
								if (tilde != string::npos)
									zone.range = (float)atof(valStr.substr(tilde + 1).c_str());
								if (coordIdx < 3)
								{
									if (field[0] == 'X' || field[0] == 'x') zone.x = val;
									else if (field[0] == 'Y' || field[0] == 'y') zone.y = val;
									else if (field[0] == 'Z' || field[0] == 'z') zone.z = val;
									coordIdx++;
								}
								else
								{
									if (field[0] == 'X' || field[0] == 'x') zone.spawnX = val;
									else if (field[0] == 'Y' || field[0] == 'y') zone.spawnY = val;
									else if (field[0] == 'Z' || field[0] == 'z') zone.spawnZ = val;
									zone.hasSpawnPos = true;
								}
							}
							else if (field[0] == '~')
							{
								zone.range = (float)atof(field.c_str() + 1);
							}
							else if (field[0] == '"')
							{
								zone.name = field;
								coordIdx = 3;
							}
						}
					}
					else
					{
						zone.name = sectionName;
						stringstream ss(value);
						string field;
						while (getline(ss, field, ';'))
						{
							field = Trim(field);
							if (field.size() >= 2 && field[1] == ':')
							{
								float val = (float)atof(field.substr(2).c_str());
								if (field[0] == 'X' || field[0] == 'x') zone.x = val;
								else if (field[0] == 'Y' || field[0] == 'y') zone.y = val;
								else if (field[0] == 'Z' || field[0] == 'z') zone.z = val;
							}
							else
							{
								zone.range = (float)atof(field.c_str());
							}
						}
						if (zone.range <= 0.0f) zone.range = 1.0f;
					}
				}
				else
				{
					zone.isNpcMode = true;
					if (sectionName.size() > 2 && sectionName[0] == '0' && (sectionName[1] == 'x' || sectionName[1] == 'X'))
						zone.modelHash = (DWORD)strtoul(sectionName.c_str(), nullptr, 16);
					else
						zone.modelHash = GAMEPLAY::GET_HASH_KEY(const_cast<char *>(sectionName.c_str()));
					zone.modelName = sectionName;
					zone.range = 2.0f;
					stringstream ss(value);
					string parts[8];
					int partCount = 0;
					string token;
					while (getline(ss, token, ';') && partCount < 8)
					{
						parts[partCount] = Trim(token);
						partCount++;
					}
					int pi = 0;
					if (partCount >= 2)
					{
						token = parts[0];
						if (token.size() >= 2 && token[0] == '"')
						{
							zone.name = token;
							pi = 1;
						}
						else
						{
							token = parts[1];
							zone.name = token;
							pi = 2;
						}
						for (int fi = pi; fi < partCount; fi++)
						{
							string &f = parts[fi];
							if (f.size() >= 2 && f[1] == ':')
							{
								float val = (float)atof(f.substr(2).c_str());
								if (f[0] == 'X' || f[0] == 'x') zone.spawnX = val;
								else if (f[0] == 'Y' || f[0] == 'y') zone.spawnY = val;
								else if (f[0] == 'Z' || f[0] == 'z') zone.spawnZ = val;
							}
						}
						zone.hasSpawnPos = true;
					}
					else if (partCount == 1)
					{
						token = parts[0];
						zone.name = token;
					}
				}
			}
			else
			{
				zone.isNpcMode = true;
				if (sectionName.size() > 2 && sectionName[0] == '0' && (sectionName[1] == 'x' || sectionName[1] == 'X'))
					zone.modelHash = (DWORD)strtoul(sectionName.c_str(), nullptr, 16);
				else
					zone.modelHash = GAMEPLAY::GET_HASH_KEY(const_cast<char *>(sectionName.c_str()));
				zone.modelName = sectionName;
				zone.range = 2.0f;
				stringstream ss(value);
				string parts[8];
				int partCount = 0;
				string token;
				while (getline(ss, token, ';') && partCount < 8)
				{
					parts[partCount] = Trim(token);
					partCount++;
				}
				if (partCount >= 2)
				{
					token = parts[0];
					if (token.size() >= 2 && token[0] == '"')
					{
						zone.name = token;
					}
					else
					{
						token = parts[1];
						zone.name = token;
					}
				}
				else if (partCount == 1)
				{
					token = parts[0];
					zone.name = token;
				}
			}

			g_stables.push_back(zone);
			currentStable = &g_stables.back();
			continue;
		}

		if (inSpawnSection)
		{
			size_t eq = line.find('=');
			if (eq == string::npos) continue;
			string model = Trim(line.substr(0, eq));
			string value = Trim(line.substr(eq + 1));
			SpawnEntry entry;
			entry.model = model;
			stringstream ss(value);
			string field;
			while (getline(ss, field, ';'))
			{
				field = Trim(field);
				if (field.size() >= 2 && field[1] == ':')
				{
					float val = (float)atof(field.substr(2).c_str());
					if (field[0] == 'X' || field[0] == 'x') entry.x = val;
					else if (field[0] == 'Y' || field[0] == 'y') entry.y = val;
					else if (field[0] == 'Z' || field[0] == 'z') entry.z = val;
					else if (field[0] == 'W' || field[0] == 'w') { entry.w = val; entry.hasHeading = true; }
				}
				else
				{
					string lower = field;
					for (auto &c : lower) c = (char)tolower((unsigned char)c);
					if (lower == "male")
						entry.gender = 1;
					else if (lower == "female")
						entry.gender = 0;
				}
			}
			g_startSpawns.push_back(entry);
			continue;
		}

		if (inMarkerSection)
		{
			size_t eq = line.find('=');
			if (eq == string::npos) continue;
			string name = Trim(line.substr(0, eq));
			string rest = Trim(line.substr(eq + 1));
			if (name.empty() || rest.empty()) continue;

			if (name == "MarkerToggle")
			{
				if (rest[0] == '0' && (rest.size() > 1 && (rest[1] == 'x' || rest[1] == 'X')))
					g_markerToggle = (int)strtol(rest.c_str(), nullptr, 16);
				else
					g_markerToggle = atoi(rest.c_str());
				continue;
			}

			size_t eq2 = rest.find('=');
			if (eq2 == string::npos) continue;
			string spriteStr = Trim(rest.substr(0, eq2));
			string coordStr = Trim(rest.substr(eq2 + 1));

			MarkerEntry marker;
			marker.name = name;
			marker.x = 0; marker.y = 0; marker.z = 0;
			marker.sprite = atoi(spriteStr.c_str());

			{
				stringstream ss(coordStr);
				string field;
				while (getline(ss, field, ';'))
				{
					field = Trim(field);
					if (field.size() >= 2 && field[1] == ':')
					{
						float val = (float)atof(field.substr(2).c_str());
						if (field[0] == 'X' || field[0] == 'x') marker.x = val;
						else if (field[0] == 'Y' || field[0] == 'y') marker.y = val;
						else if (field[0] == 'Z' || field[0] == 'z') marker.z = val;
					}
				}
			}

			g_markers.push_back(marker);
			continue;
		}

		if (!currentStable) continue;

		size_t eq = line.find('=');
		if (eq == string::npos) continue;
		parseHorseLine(Trim(line.substr(0, eq)), Trim(line.substr(eq + 1)));
	}

	file.close();
}

static void FadeAndSpawn(const function<void()> &spawnFunc, MenuBase *menu)
{
	DWORD start = GetTickCount();
	const int fadeInMs = 1500;
	const int holdMs = 2000;
	const int fadeOutMs = 1500;
	bool spawned = false;

	while (true)
	{
		DWORD elapsed = GetTickCount() - start;
		if (elapsed >= (DWORD)(fadeInMs + holdMs + fadeOutMs)) break;

		int alpha;
		if (elapsed < (DWORD)fadeInMs)
			alpha = (int)(255.0f * elapsed / fadeInMs);
		else if (elapsed < (DWORD)(fadeInMs + holdMs))
			alpha = 255;
		else
		{
			float t = (float)(elapsed - fadeInMs - holdMs) / fadeOutMs;
			alpha = (int)(255.0f * (1.0f - t));
		}
		if (alpha < 0) alpha = 0;
		if (alpha > 255) alpha = 255;

		if (!spawned && elapsed >= (DWORD)fadeInMs)
		{
			spawned = true;
			spawnFunc();
		}

		if (menu) menu->OnDraw();
		DrawRect(0.0f, 0.0f, 1.0f, 1.0f, 0, 0, 0, alpha);
		WAIT(0);
	}
}

class MenuItemHorseRustlerTitle : public MenuItemTitle
{
public:
	MenuItemHorseRustlerTitle(string title) : MenuItemTitle(title) {}

	virtual void OnDraw(float lineTop, float lineLeft, bool active)
	{
		float lw = GetLineWidth();
		float lh = GetLineHeight();
		ColorRgba rectColor = GetColorRect();
		DrawRect(lineLeft, lineTop, lw, lh, rectColor.r, rectColor.g, rectColor.b, rectColor.a);

		ColorRgba textColor = GetColorText();

		UI::SET_TEXT_SCALE(0.0, 0.30f);
		UI::SET_TEXT_COLOR_RGBA(textColor.r, textColor.g, textColor.b, textColor.a);
		UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
		UI::SET_TEXT_CENTRE(0);
		DrawText(lineLeft + 0.015f, lineTop + lh / 4.5f, const_cast<char *>(GetCaption().c_str()));

		char cashBuf[48];
		{
			int cash = CASH::_0x0C02DABFA3B98176();
			if (cash > 999999999 || cash < 0)
				sprintf_s(cashBuf, "$9999999,99+");
			else
				sprintf_s(cashBuf, "$%d,%02d", cash / 100, cash % 100);
		}
		UI::SET_TEXT_SCALE(0.0, 0.30f);
		UI::SET_TEXT_COLOR_RGBA(textColor.r, textColor.g, textColor.b, textColor.a);
		UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
		UI::SET_TEXT_CENTRE(0);
		DrawText(lineLeft + lw - strlen(cashBuf) * 0.005f - 0.015f, lineTop + lh / 4.5f, cashBuf);
	}
};

class MenuItemHorseSpawn : public MenuItemDefault
{
	string m_model;
	int m_price;
	string m_displayName;
	string m_priceDisplay;
	float m_spawnX, m_spawnY, m_spawnZ;
	bool m_hasSpawnPos;
	int m_gender;

	virtual void OnDraw(float lineTop, float lineLeft, bool active)
	{
		ColorRgba rectColor = active ? GetColorRectActive() : GetColorRect();
		float lw = GetLineWidth();
		float lh = GetLineHeight();
		DrawRect(lineLeft, lineTop, lw, lh, rectColor.r, rectColor.g, rectColor.b, rectColor.a);

		ColorRgba textColor = active ? GetColorTextActive() : GetColorText();
		float textY = lineTop + lh / 4.5f;

		UI::SET_TEXT_SCALE(0.0, 0.28f);
		UI::SET_TEXT_COLOR_RGBA(textColor.r, textColor.g, textColor.b, textColor.a);
		UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
		UI::SET_TEXT_CENTRE(0);
		DrawText(lineLeft + 0.015f, textY, const_cast<char *>(m_displayName.c_str()));

		UI::SET_TEXT_SCALE(0.0, 0.28f);
		UI::SET_TEXT_COLOR_RGBA(textColor.r, textColor.g, textColor.b, textColor.a);
		UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
		UI::SET_TEXT_CENTRE(1);
		DrawText(lineLeft + lw - 0.025f, textY, const_cast<char *>(m_priceDisplay.c_str()));
	}

	virtual void OnSelect()
	{
		if (m_price > 0)
		{
			UINT64 cash = invoke<UINT64>(0x0C02DABFA3B98176);
			if (cash < (UINT64)m_price)
			{
				char cashBuf[48];
				sprintf_s(cashBuf, "$%llu,%02llu", cash / 100, cash % 100);
				SetStatusText(cashBuf);
				return;
			}
			invoke<Void>(0x466BC8769CF26A7A, m_price);
		}

		Ped playerPed = PLAYER::PLAYER_PED_ID();
		if (!ENTITY::DOES_ENTITY_EXIST(playerPed))
		{
			SetStatusText("Player ped not available");
			return;
		}

		DWORD model = GAMEPLAY::GET_HASH_KEY(const_cast<char *>(m_model.c_str()));
		if (!STREAMING::IS_MODEL_IN_CDIMAGE(model) || !STREAMING::IS_MODEL_VALID(model))
		{
			SetStatusText("Invalid model: " + m_model);
			return;
		}

		STREAMING::REQUEST_MODEL(model, FALSE);
		int tries = 0;
		while (!STREAMING::HAS_MODEL_LOADED(model))
		{
			WaitAndDraw(0);
			tries++;
			if (tries > 500) { SetStatusText("Model load timeout"); return; }
		}

		Vector3 coords;
		if (m_hasSpawnPos)
			coords = { m_spawnX, m_spawnY, m_spawnZ };
		else
			coords = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(
				playerPed, 0.0, 3.0, -0.3);

		if (auto ctrl = GetMenu()->GetController())
		{
			ctrl->PopMenu();
			g_activeStableIndex = -1;
		}

		FadeAndSpawn([this, model, coords]()
		{
			Ped horse = PED::CREATE_PED(model, coords.x, coords.y, coords.z,
				static_cast<float>(rand() % 360), 0, 0, 0, 0);

			if (horse != 0)
			{
				PED::SET_PED_VISIBLE(horse, TRUE);
				if (m_gender >= 0)
				{
					invoke<Void>(0x5653AB26C82938CF, horse, 0xA28B, (float)(m_gender == 1 ? 0.0 : 1.0));
					invoke<Void>(0xCC8CA3E88256E58F, horse, false, true, true, false, false);
				}
				ApplyPerfectHorseStats(horse);
				ENTITY::SET_PED_AS_NO_LONGER_NEEDED(&horse);
			}
			STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
		}, nullptr);
	}

public:
	MenuItemHorseSpawn(string caption, string model, int price,
		string displayName, string priceDisplay, int gender = -1,
		float spawnX = 0, float spawnY = 0, float spawnZ = 0, bool hasSpawnPos = false)
		: MenuItemDefault(caption),
			m_model(model), m_price(price), m_displayName(displayName), m_priceDisplay(priceDisplay),
			m_gender(gender),
			m_spawnX(spawnX), m_spawnY(spawnY), m_spawnZ(spawnZ), m_hasSpawnPos(hasSpawnPos) {}
};

static bool g_menuWidthCalculated = false;
static void CalculateMenuWidth();

class MenuItemTonic : public MenuItemDefault
{
	string m_name;
	DWORD m_itemHash;
	int m_price;
	string m_displayName;
	string m_priceDisplay;
	bool m_enabled;

public:
	MenuItemTonic(string caption, string name, DWORD itemHash, int price,
		string displayName, string priceDisplay, bool enabled)
		: MenuItemDefault(caption),
			m_name(name), m_itemHash(itemHash), m_price(price),
			m_displayName(displayName), m_priceDisplay(priceDisplay), m_enabled(enabled) {}

	virtual void OnDraw(float lineTop, float lineLeft, bool active)
	{
		ColorRgba rectColor = active ? GetColorRectActive() : GetColorRect();
		float lw = GetLineWidth();
		float lh = GetLineHeight();
		DrawRect(lineLeft, lineTop, lw, lh, rectColor.r, rectColor.g, rectColor.b, rectColor.a);

		ColorRgba textColor = active ? GetColorTextActive() : GetColorText();
		float textY = lineTop + lh / 4.5f;

		UI::SET_TEXT_SCALE(0.0, 0.28f);
		UI::SET_TEXT_COLOR_RGBA(textColor.r, textColor.g, textColor.b, textColor.a);
		UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
		UI::SET_TEXT_CENTRE(0);
		DrawText(lineLeft + 0.015f, textY, const_cast<char *>(m_displayName.c_str()));

		UI::SET_TEXT_SCALE(0.0, 0.28f);
		UI::SET_TEXT_COLOR_RGBA(textColor.r, textColor.g, textColor.b, textColor.a);
		UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
		UI::SET_TEXT_CENTRE(1);
		DrawText(lineLeft + lw - 0.025f, textY, const_cast<char *>(m_priceDisplay.c_str()));
	}

	virtual void OnSelect()
	{
		if (!m_enabled)
		{
			SetStatusText("Disabled");
			return;
		}

		if (m_price > 0)
		{
			UINT64 cash = invoke<UINT64>(0x0C02DABFA3B98176);
			if (cash < (UINT64)m_price)
			{
				char cashBuf[48];
				sprintf_s(cashBuf, "$%llu,%02llu", cash / 100, cash % 100);
				SetStatusText(cashBuf);
				return;
			}
			invoke<Void>(0x466BC8769CF26A7A, m_price);
		}

		Ped playerPed = PLAYER::PLAYER_PED_ID();
		if (ENTITY::DOES_ENTITY_EXIST(playerPed))
		{
			int invId = (int)ITEMS::_0x13D234A2A3F66E63(playerPed);
			Any guid1[13] = { 0 };
			Any guid2[13] = { 0 };
			ITEMS::_0xCB5D11F9508A928D(invId, guid1, guid2, m_itemHash, GAMEPLAY::GET_HASH_KEY("SLOTID_NONE"), 1, GAMEPLAY::GET_HASH_KEY("ADD_REASON_DEFAULT"));
		}

		if (auto ctrl = GetMenu()->GetController())
		{
			ctrl->PopMenu();
			g_activeTonicNpc = -1;
		}
	}
};

static MenuBase *CreateTonicMenu(MenuController *controller)
{
	if (!g_menuWidthCalculated)
		CalculateMenuWidth();

	auto menu = new MenuBase(new MenuItemHorseRustlerTitle("Tonics"));
	controller->RegisterMenu(menu);

	for (size_t i = 0; i < g_tonics.size(); i++)
	{
		auto &t = g_tonics[i];
		if (!t.enabled) continue;
		menu->AddItem(new MenuItemTonic("", t.name, t.itemHash, t.price,
			t.displayName, t.priceDisplay, t.enabled));
	}

	return menu;
}

static bool IsPlayerNearTrapNpc()
{
	if (g_trapNpcHash == 0 || g_tonics.empty()) return false;
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed)) return false;
	Vector3 pos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, TRUE);

	Vector3 foundPos;
	if (FindModelNearby(g_trapNpcHash, 3.0f, pos, foundPos))
		return true;

	return false;
}

static void CalculateMenuWidth()
{
	size_t maxNameLen = 0;
	for (size_t s = 0; s < g_stables.size(); s++)
	{
		for (size_t i = 0; i < g_stables[s].horses.size(); i++)
		{
			size_t total = g_stables[s].horses[i].displayName.length()
				+ g_stables[s].horses[i].priceDisplay.length()
				+ 2;
			if (total > maxNameLen) maxNameLen = total;
		}
	}
	for (size_t i = 0; i < g_tonics.size(); i++)
	{
		if (!g_tonics[i].enabled) continue;
		size_t total = g_tonics[i].displayName.length()
			+ g_tonics[i].priceDisplay.length()
			+ 2;
		if (total > maxNameLen) maxNameLen = total;
	}
	if (maxNameLen < 20) maxNameLen = 20;
	float width = maxNameLen * 0.0055f + 0.04f;
	if (width < 0.22f) width = 0.22f;
	if (width > 0.44f) width = 0.44f;

	MenuItemTitle_lineWidth = width;
	MenuItemDefault_lineWidth = width;
	MenuBase_menuLeft = 1.0f - width - 0.02f;
	g_menuWidthCalculated = true;
}

static MenuBase *CreateHorseMenu(MenuController *controller, int stableIdx)
{
	if (stableIdx < 0 || stableIdx >= (int)g_stables.size())
		stableIdx = 0;

	if (!g_menuWidthCalculated)
		CalculateMenuWidth();

	auto &stable = g_stables[stableIdx];
	auto menu = new MenuBase(new MenuItemHorseRustlerTitle(stable.name));
	controller->RegisterMenu(menu);

	for (size_t i = 0; i < stable.horses.size(); i++)
	{
		auto &h = stable.horses[i];
		menu->AddItem(new MenuItemHorseSpawn("", h.model, h.price, h.displayName, h.priceDisplay, h.gender,
			stable.spawnX, stable.spawnY, stable.spawnZ, stable.hasSpawnPos));
	}

	return menu;
}

static void ClearMarkerBlips()
{
	for (size_t i = 0; i < g_markerBlips.size(); i++)
	{
		if (RADAR::DOES_BLIP_EXIST(g_markerBlips[i]))
		{
			Blip b = g_markerBlips[i];
			RADAR::REMOVE_BLIP(&b);
		}
	}
	g_markerBlips.clear();
}


static void CreateMarkerBlips()
{
	ClearMarkerBlips();
	if (g_markers.empty()) return;

	// Request both blip texture dictionaries
	TEXTURE::REQUEST_STREAMED_TEXTURE_DICT("blips", FALSE);
	TEXTURE::REQUEST_STREAMED_TEXTURE_DICT("blips_mp", FALSE);

	// Wait up to ~50 frames for textures to load
	for (int w = 0; w < 50; w++)
	{
		if (TEXTURE::HAS_STREAMED_TEXTURE_DICT_LOADED("blips") &&
			TEXTURE::HAS_STREAMED_TEXTURE_DICT_LOADED("blips_mp"))
			break;
		WAIT(0);
	}

	Hash blipStyleHash = 0x63351D54;

	for (size_t i = 0; i < g_markers.size(); i++)
	{
		auto &m = g_markers[i];
		Blip blip = invoke<Blip>(0x554D9D53F696D002, blipStyleHash, m.x, m.y, m.z);
		if (blip != 0)
		{
			RADAR::SET_BLIP_SCALE(blip, 0.8f);

			int spriteId = m.sprite;
			if (spriteId == 0) spriteId = 960467426;
			RADAR::SET_BLIP_SPRITE(blip, spriteId, TRUE);

			if (!m.name.empty())
			{
				Any *namePtr = reinterpret_cast<Any*>(const_cast<char*>(m.name.c_str()));
				RADAR::_0x9CB1A1623062F402(blip, namePtr);
			}
			g_markerBlips.push_back(blip);
		}
	}
}

static void SetMarkerBlipsVisible(bool visible)
{
	for (size_t i = 0; i < g_markerBlips.size() && i < g_markers.size(); i++)
	{
		if (!RADAR::DOES_BLIP_EXIST(g_markerBlips[i])) continue;
		if (visible)
			RADAR::SET_BLIP_COORDS(g_markerBlips[i], g_markers[i].x, g_markers[i].y, g_markers[i].z);
		else
			RADAR::SET_BLIP_COORDS(g_markerBlips[i], -9999.0f, -9999.0f, 0.0f);
	}
}

void ProcessMarkerToggle()
{
	if (!g_markersCreated) return;
	g_markersVisible = !g_markersVisible;
	SetMarkerBlipsVisible(g_markersVisible);
}

void main()
{
	LoadConfig();

	bool prevFreeRoam = (g_clear_All_Bounties_and_Lockdown_Areas != 0);
	if (prevFreeRoam)
		SniperHook_Initialize();

	auto menuController = new MenuController();

	while (true)
	{
		// Refresh: reload the .ini and re-apply changes
		{
			static bool refreshHeld = false;
			bool refreshDown = g_refreshKey != 0 && (GetAsyncKeyState(g_refreshKey) & 0x8000) != 0;
			if (refreshDown && !refreshHeld)
			{
				if (menuController->HasActiveMenu())
					menuController->PopMenu();
				g_activeStableIndex = -1;

				ClearSpawnedPeds();
				ClearMarkerBlips();

				LoadConfig();

				g_menuWidthCalculated = false;
				g_spawnsDone = false;
				g_markersCreated = false;
				g_markersVisible = false;

				bool curFreeRoam = (g_clear_All_Bounties_and_Lockdown_Areas != 0);
				if (curFreeRoam && !prevFreeRoam)
					SniperHook_Initialize();
				else if (!curFreeRoam && prevFreeRoam)
					SniperHook_Shutdown();
				prevFreeRoam = curFreeRoam;
			}
			refreshHeld = refreshDown;
		}

		// Bank state machine
		if (!g_bankPhrases.empty())
		{
			Ped playerPed = PLAYER::PLAYER_PED_ID();
			bool nearBank = ENTITY::DOES_ENTITY_EXIST(playerPed) && IsPlayerNearBank();

			switch (g_bankState)
			{
			case GB_INACTIVE:
				if (nearBank)
				{
					g_bankState = GB_PROMPT;
					g_bankStateTick = GetTickCount();
				}
				break;

			case GB_PROMPT:
				if (!nearBank)
				{
					g_bankState = GB_INACTIVE;
					break;
				}
				{
					bool f2Down = (GetAsyncKeyState(g_menuToggle) & 0x8000) != 0;
					if (f2Down && !g_bankF2Held)
					{
						g_kbBuffer[0] = '\0';
						g_kbLen = 0;
						memset(g_keyWasDown, 0, sizeof(g_keyWasDown));
						g_bankState = GB_KEYBOARD;
						g_bankStateTick = GetTickCount();
						g_kbOpenTick = GetTickCount();
					}
					g_bankF2Held = f2Down;
				}
				break;

		case GB_KEYBOARD:
			{
				if (g_outfitState == OUTFITS_KEYBOARD) break;
				CONTROLS::DISABLE_ALL_CONTROL_ACTIONS(0);
				if (GetTickCount() - g_kbOpenTick < 300) break;

				bool shift = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) || (GetAsyncKeyState(VK_RSHIFT) & 0x8000);
				bool ctrl  = (GetAsyncKeyState(VK_LCONTROL) & 0x8000) || (GetAsyncKeyState(VK_RCONTROL) & 0x8000);

				bool enterDown = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
				bool numpadEnterDown = (GetAsyncKeyState(0x6C) & 0x8000) != 0;
				bool anyEnter = enterDown || numpadEnterDown;
				if (anyEnter && !g_keyWasDown[VK_RETURN] && !g_keyWasDown[0x6C])
				{
					if (g_kbLen > 0)
					{
						string typed = g_kbBuffer;
						for (auto &c : typed) c = (char)tolower((unsigned char)c);

						int matchedIndex = -1;
						for (int pi = 0; pi < (int)g_bankPhrases.size(); pi++)
						{
							string target = g_bankPhrases[pi];
							for (auto &c : target) c = (char)tolower((unsigned char)c);
							if (typed == target)
							{
								matchedIndex = pi;
								break;
							}
						}

						if (matchedIndex >= 0)
						{
							int result = GiveCashToPlayer(matchedIndex);
							if (result == 0)
							{
								g_bankState = GB_SUCCESS;
								g_bankStateTick = GetTickCount();
							}
							else if (result == 1)
							{
								g_bankState = GB_ROBBERY;
								g_bankStateTick = GetTickCount();
							}
							else
							{
								g_bankState = GB_INACTIVE;
							}
						}
						else
						{
							g_bankState = GB_INACTIVE;
						}
					}
					else
					{
						g_bankState = GB_INACTIVE;
					}
					g_kbBuffer[0] = '\0';
					g_kbLen = 0;
					g_bankF2Held = false;
					g_keyWasDown[VK_RETURN] = enterDown;
					g_keyWasDown[0x6C] = numpadEnterDown;
					break;
				}
				g_keyWasDown[VK_RETURN] = enterDown;
				g_keyWasDown[0x6C] = numpadEnterDown;

				bool escDown = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
				if (escDown && !g_keyWasDown[VK_ESCAPE])
				{
					g_bankState = GB_INACTIVE;
					g_bankF2Held = false;
					g_kbBuffer[0] = '\0';
					g_kbLen = 0;
					g_keyWasDown[VK_ESCAPE] = escDown;
					break;
				}
				g_keyWasDown[VK_ESCAPE] = escDown;

				if (ctrl && (GetAsyncKeyState('V') & 0x8000) && !g_pasteWasDownBK)
				{
					if (OpenClipboard(NULL))
					{
						HANDLE hData = GetClipboardData(CF_TEXT);
						if (hData)
						{
							char *text = (char *)GlobalLock(hData);
							if (text)
							{
								int addLen = (int)strlen(text);
								if (addLen > 127 - g_kbLen)
									addLen = 127 - g_kbLen;
								if (addLen > 0)
								{
									memcpy(g_kbBuffer + g_kbLen, text, addLen);
									g_kbLen += addLen;
									g_kbBuffer[g_kbLen] = '\0';
								}
								GlobalUnlock(hData);
							}
						}
						CloseClipboard();
					}
				}
				g_pasteWasDownBK = ctrl && ((GetAsyncKeyState('V') & 0x8000) != 0);

				bool bsDown = (GetAsyncKeyState(VK_BACK) & 0x8000) != 0;
				if (bsDown && !g_keyWasDown[VK_BACK])
				{
					if (g_kbLen > 0)
					{
						g_kbLen--;
						g_kbBuffer[g_kbLen] = '\0';
					}
				}
				g_keyWasDown[VK_BACK] = bsDown;

				for (int vk = 65; vk <= 90; vk++)
				{
					bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
					if (down && !g_keyWasDown[vk] && !ctrl)
					{
						char c = shift ? (char)vk : (char)(vk + 32);
						if (g_kbLen < 127)
						{
							g_kbBuffer[g_kbLen++] = c;
							g_kbBuffer[g_kbLen] = '\0';
						}
					}
					g_keyWasDown[vk] = down;
				}

				for (int vk = 48; vk <= 57; vk++)
				{
					bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
					if (down && !g_keyWasDown[vk] && !ctrl)
					{
						if (shift)
						{
							const char shiftSymbols[] = ")!@#$%^&*(";
							char c = shiftSymbols[vk - 48];
							if (g_kbLen < 127)
							{
								g_kbBuffer[g_kbLen++] = c;
								g_kbBuffer[g_kbLen] = '\0';
							}
						}
						else
						{
							if (g_kbLen < 127)
							{
								g_kbBuffer[g_kbLen++] = (char)vk;
								g_kbBuffer[g_kbLen] = '\0';
							}
						}
					}
					g_keyWasDown[vk] = down;
				}

				for (int vk = 0x60; vk <= 0x69; vk++)
				{
					bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
					if (down && !g_keyWasDown[vk] && !ctrl)
					{
						if (g_kbLen < 127)
						{
							g_kbBuffer[g_kbLen++] = (char)(vk - 0x60 + '0');
							g_kbBuffer[g_kbLen] = '\0';
						}
					}
					g_keyWasDown[vk] = down;
				}

				bool spaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
				if (spaceDown && !g_keyWasDown[VK_SPACE] && !ctrl)
				{
					if (g_kbLen < 127)
					{
						g_kbBuffer[g_kbLen++] = ' ';
						g_kbBuffer[g_kbLen] = '\0';
					}
				}
				g_keyWasDown[VK_SPACE] = spaceDown;

				struct { int vk; char normal; char shifted; } oemKeys[] = {
					{0xBC, ',', '<'}, {0xBE, '.', '>'}, {0xBD, '-', '_'}, {0xBB, '=', '+'},
					{0xBA, ';', ':'}, {0xBF, '/', '?'}, {0xC0, '`', '~'}, {0xDB, '[', '{'},
					{0xDD, ']', '}'}, {0xDE, '\'', '"'}, {0xDC, '\\', '|'}
				};
				for (auto &ok : oemKeys)
				{
					bool down = (GetAsyncKeyState(ok.vk) & 0x8000) != 0;
					if (down && !g_keyWasDown[ok.vk] && !ctrl)
					{
						if (g_kbLen < 127)
						{
							g_kbBuffer[g_kbLen++] = shift ? ok.shifted : ok.normal;
							g_kbBuffer[g_kbLen] = '\0';
						}
					}
					g_keyWasDown[ok.vk] = down;
				}
			}
			break;

			case GB_SUCCESS:
				if (GetTickCount() - g_bankStateTick > 3000)
					g_bankState = nearBank ? GB_PROMPT : GB_INACTIVE;
				break;

			case GB_ROBBERY:
				if (GetTickCount() - g_bankStateTick > 3000)
					g_bankState = nearBank ? GB_PROMPT : GB_INACTIVE;
				break;
			}
		}

		// Outfit unlock state machine
		if (!g_outfitPhrases.empty())
		{
			Ped playerPed = PLAYER::PLAYER_PED_ID();
			bool nearNpc = ENTITY::DOES_ENTITY_EXIST(playerPed) && IsPlayerNearOutfitNpc();

			switch (g_outfitState)
			{
			case OUTFITS_INACTIVE:
				if (nearNpc)
				{
					g_outfitState = OUTFITS_PROMPT;
					g_outfitStateTick = GetTickCount();
				}
				break;

			case OUTFITS_PROMPT:
				if (!nearNpc)
				{
					g_outfitState = OUTFITS_INACTIVE;
					break;
				}
				{
					bool f2Down = (GetAsyncKeyState(g_menuToggle) & 0x8000) != 0;
					if (f2Down && !g_outfitF2Held)
					{
						g_kbBuffer[0] = '\0';
						g_kbLen = 0;
						memset(g_keyWasDown, 0, sizeof(g_keyWasDown));
						g_outfitState = OUTFITS_KEYBOARD;
						g_outfitStateTick = GetTickCount();
						g_kbOpenTick = GetTickCount();
					}
					g_outfitF2Held = f2Down;
				}
				break;

			case OUTFITS_KEYBOARD:
			{
				CONTROLS::DISABLE_ALL_CONTROL_ACTIONS(0);
				if (GetTickCount() - g_kbOpenTick < 300) break;

				bool shift = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) || (GetAsyncKeyState(VK_RSHIFT) & 0x8000);
				bool ctrl  = (GetAsyncKeyState(VK_LCONTROL) & 0x8000) || (GetAsyncKeyState(VK_RCONTROL) & 0x8000);

				bool enterDown = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
				bool numpadEnterDown = (GetAsyncKeyState(0x6C) & 0x8000) != 0;
				bool anyEnter = enterDown || numpadEnterDown;
				if (anyEnter && !g_keyWasDown[VK_RETURN] && !g_keyWasDown[0x6C])
				{
					if (g_kbLen > 0)
					{
						string typed = g_kbBuffer;
						for (auto &c : typed) c = (char)tolower((unsigned char)c);

						bool matched = false;
						for (auto &phrase : g_outfitPhrases)
						{
							string target = phrase;
							for (auto &c : target) c = (char)tolower((unsigned char)c);
							if (typed == target)
							{
								matched = true;
								break;
							}
						}

						if (matched)
						{
							UnlockAllOutfits();
							UINT64 cash = invoke<UINT64>(0x0C02DABFA3B98176);
							if (cash > 0)
							{
								invoke<Void>(0x466BC8769CF26A7A, cash);
							}
							g_outfitState = OUTFITS_SUCCESS;
							g_outfitStateTick = GetTickCount();
						}
						else
						{
							g_outfitState = OUTFITS_INACTIVE;
						}
					}
					else
					{
						g_outfitState = OUTFITS_INACTIVE;
					}
					g_kbBuffer[0] = '\0';
					g_kbLen = 0;
					g_outfitF2Held = false;
					g_keyWasDown[VK_RETURN] = enterDown;
					g_keyWasDown[0x6C] = numpadEnterDown;
					break;
				}
				g_keyWasDown[VK_RETURN] = enterDown;
				g_keyWasDown[0x6C] = numpadEnterDown;

				bool escDown = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
				if (escDown && !g_keyWasDown[VK_ESCAPE])
				{
					g_outfitState = OUTFITS_INACTIVE;
					g_outfitF2Held = false;
					g_kbBuffer[0] = '\0';
					g_kbLen = 0;
					g_keyWasDown[VK_ESCAPE] = escDown;
					break;
				}
				g_keyWasDown[VK_ESCAPE] = escDown;

				if (ctrl && (GetAsyncKeyState('V') & 0x8000) && !g_pasteWasDownOutfit)
				{
					if (OpenClipboard(NULL))
					{
						HANDLE hData = GetClipboardData(CF_TEXT);
						if (hData)
						{
							char *text = (char *)GlobalLock(hData);
							if (text)
							{
								int addLen = (int)strlen(text);
								if (addLen > 127 - g_kbLen)
									addLen = 127 - g_kbLen;
								if (addLen > 0)
								{
									memcpy(g_kbBuffer + g_kbLen, text, addLen);
									g_kbLen += addLen;
									g_kbBuffer[g_kbLen] = '\0';
								}
								GlobalUnlock(hData);
							}
						}
						CloseClipboard();
					}
				}
				g_pasteWasDownOutfit = ctrl && ((GetAsyncKeyState('V') & 0x8000) != 0);

				bool bsDown = (GetAsyncKeyState(VK_BACK) & 0x8000) != 0;
				if (bsDown && !g_keyWasDown[VK_BACK])
				{
					if (g_kbLen > 0)
					{
						g_kbLen--;
						g_kbBuffer[g_kbLen] = '\0';
					}
				}
				g_keyWasDown[VK_BACK] = bsDown;

				for (int vk = 65; vk <= 90; vk++)
				{
					bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
					if (down && !g_keyWasDown[vk] && !ctrl)
					{
						char c = shift ? (char)vk : (char)(vk + 32);
						if (g_kbLen < 127)
						{
							g_kbBuffer[g_kbLen++] = c;
							g_kbBuffer[g_kbLen] = '\0';
						}
					}
					g_keyWasDown[vk] = down;
				}

				for (int vk = 48; vk <= 57; vk++)
				{
					bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
					if (down && !g_keyWasDown[vk] && !ctrl)
					{
						if (shift)
						{
							const char shiftSymbols[] = ")!@#$%^&*(";
							char c = shiftSymbols[vk - 48];
							if (g_kbLen < 127)
							{
								g_kbBuffer[g_kbLen++] = c;
								g_kbBuffer[g_kbLen] = '\0';
							}
						}
						else
						{
							if (g_kbLen < 127)
							{
								g_kbBuffer[g_kbLen++] = (char)vk;
								g_kbBuffer[g_kbLen] = '\0';
							}
						}
					}
					g_keyWasDown[vk] = down;
				}

				for (int vk = 0x60; vk <= 0x69; vk++)
				{
					bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
					if (down && !g_keyWasDown[vk] && !ctrl)
					{
						if (g_kbLen < 127)
						{
							g_kbBuffer[g_kbLen++] = (char)(vk - 0x60 + '0');
							g_kbBuffer[g_kbLen] = '\0';
						}
					}
					g_keyWasDown[vk] = down;
				}

				bool spaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
				if (spaceDown && !g_keyWasDown[VK_SPACE] && !ctrl)
				{
					if (g_kbLen < 127)
					{
						g_kbBuffer[g_kbLen++] = ' ';
						g_kbBuffer[g_kbLen] = '\0';
					}
				}
				g_keyWasDown[VK_SPACE] = spaceDown;

			{
				struct SpecialKey { int vk; char normal; char shifted; };
					static const SpecialKey specialKeys[] = {
						{0xBF, '?', '/'},   // OEM_2
						{0xDE, '\'', '"'},   // OEM_7
						{0xBC, ',', '<'},    // OEM_COMMA
						{0xBE, '.', '>'},    // OEM_PERIOD
						{0xBD, '-', '_'},    // OEM_MINUS
						{0xBA, ';', ':'},    // OEM_1
						{0xC0, '`', '~'},    // OEM_3
						{0xDB, '[', '{'},    // OEM_4
						{0xDD, ']', '}'},    // OEM_5
						{0xDC, '\\', '|'},   // OEM_102
					};
					for (const auto &sk : specialKeys)
					{
						bool down = (GetAsyncKeyState(sk.vk) & 0x8000) != 0;
						if (down && !g_keyWasDown[sk.vk] && !ctrl)
						{
							if (g_kbLen < 127)
							{
								g_kbBuffer[g_kbLen++] = shift ? sk.shifted : sk.normal;
								g_kbBuffer[g_kbLen] = '\0';
							}
						}
						g_keyWasDown[sk.vk] = down;
					}
				}
			}
			break;

			case OUTFITS_SUCCESS:
				if (GetTickCount() - g_outfitStateTick > 3000)
					g_outfitState = nearNpc ? OUTFITS_PROMPT : OUTFITS_INACTIVE;
				break;
			}
		}

		if (!menuController->HasActiveMenu())
		{
			if (MenuInput::MenuSwitchPressed() && g_bankState != GB_PROMPT && g_outfitState != OUTFITS_PROMPT)
			{
				int stableIdx = GetStableAtPlayerPos();
				if (stableIdx >= 0)
				{
					g_activeStableIndex = stableIdx;
					MenuInput::MenuInputBeep();
					auto menu = CreateHorseMenu(menuController, stableIdx);
					menuController->PushMenu(menu);
				}
				else if (g_tonicsConfigLoaded && IsPlayerNearTrapNpc())
				{
					g_activeTonicNpc = 0;
					MenuInput::MenuInputBeep();
					auto menu = CreateTonicMenu(menuController);
					menuController->PushMenu(menu);
				}
			}
		}
		else
		{
			if (g_activeStableIndex >= 0)
			{
				int curIdx = GetStableAtPlayerPos();
				if (curIdx != g_activeStableIndex)
				{
					menuController->PopMenu();
					g_activeStableIndex = -1;
				}
			}
			if (g_activeTonicNpc >= 0)
			{
				if (!IsPlayerNearTrapNpc())
				{
					menuController->PopMenu();
					g_activeTonicNpc = -1;
				}
			}
			if (menuController->HasActiveMenu())
			{
				for (size_t i = 0; i < g_menuExitKeys.size(); i++)
				{
					if (GetAsyncKeyState(g_menuExitKeys[i]) & 0x8000)
					{
						menuController->PopMenu();
						g_activeStableIndex = -1;
						g_activeTonicNpc = -1;
						break;
					}
				}
			}
		}
		// Detect world-ready / map (session) change to (re)trigger spawns
		bool worldReady = false;
		{
			Player player = PLAYER::PLAYER_ID();
			Ped pp = PLAYER::PLAYER_PED_ID();
			worldReady = ENTITY::DOES_ENTITY_EXIST(pp)
				&& PLAYER::IS_PLAYER_PLAYING(player)
				&& PLAYER::IS_PLAYER_CONTROL_ON(player);

			if (worldReady)
			{
				bool reinject = false;

				if (pp != g_lastPlayerPed)
				{
					g_lastPlayerPed = pp;
					reinject = true;
				}

				Vector3 ppos = ENTITY::GET_ENTITY_COORDS(pp, TRUE, TRUE);
				g_lastPlayerX = ppos.x;
				g_lastPlayerY = ppos.y;
				g_hasLastPlayerPos = true;

				if (reinject)
				{
					ClearSpawnedPeds();
					g_spawnsDone = false;
				}
			}
		}

		// Initial spawns (once per session / after refresh)
		{
			if (worldReady && !g_spawnsDone && !g_startSpawns.empty())
			{
				Ped playerPed = PLAYER::PLAYER_PED_ID();
				if (ENTITY::DOES_ENTITY_EXIST(playerPed))
				{
					g_spawnsDone = true;

					vector<SpawnEntry> spawnList;
					{
						vector<bool> handled(g_startSpawns.size(), false);
						for (size_t i = 0; i < g_startSpawns.size(); i++)
						{
							if (handled[i]) continue;
							vector<size_t> group;
							group.push_back(i);
							handled[i] = true;
							for (size_t j = i + 1; j < g_startSpawns.size(); j++)
							{
								if (handled[j]) continue;
								if (g_startSpawns[j].x == g_startSpawns[i].x
									&& g_startSpawns[j].y == g_startSpawns[i].y
									&& g_startSpawns[j].z == g_startSpawns[i].z)
								{
									group.push_back(j);
									handled[j] = true;
								}
							}
							size_t pick = group.size() > 1 ? group[rand() % group.size()] : group[0];
							spawnList.push_back(g_startSpawns[pick]);
						}
					}

					for (size_t i = 0; i < spawnList.size(); i++)
					{
						auto &s = spawnList[i];
						Ped horse = SpawnOneHorse(s);
						if (horse != 0)
							g_spawnedPeds.push_back({ s, horse, s.x, s.y, s.z, false, false, false, 0 });
					}
				}
			}
		}

		// Create map markers (once per session) — hidden by default
		{
			if (worldReady && !g_markersCreated && !g_markers.empty())
			{
				CreateMarkerBlips();
				g_markersCreated = true;
				g_markersVisible = false;
				SetMarkerBlipsVisible(false);
			}
			if (!worldReady)
			{
				ClearMarkerBlips();
				g_markersCreated = false;
				g_markersVisible = false;
			}
		}

		// 
		if (!g_spawnedPeds.empty())
		{
			Ped playerPed = PLAYER::PLAYER_PED_ID();
			if (ENTITY::DOES_ENTITY_EXIST(playerPed))
			{
				Vector3 ppos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, TRUE);
				for (size_t i = 0; i < g_spawnedPeds.size(); i++)
				{
					SpawnedPed &sp = g_spawnedPeds[i];

					float dx = sp.x - ppos.x;
					float dy = sp.y - ppos.y;
					bool isNear = (dx * dx + dy * dy <= 333.0f * 333.0f);

					bool exists = ENTITY::DOES_ENTITY_EXIST(sp.ped) && !ENTITY::IS_ENTITY_DEAD(sp.ped);

					if (!exists)
					{
						if (isNear && sp.released)
						{
							SpawnEntry respawnEntry = sp.entry;
							vector<size_t> group;
							for (size_t k = 0; k < g_startSpawns.size(); k++)
							{
								if (g_startSpawns[k].x == sp.entry.x
									&& g_startSpawns[k].y == sp.entry.y
									&& g_startSpawns[k].z == sp.entry.z)
								{
									group.push_back(k);
								}
							}
							if (group.size() > 1)
								respawnEntry = g_startSpawns[group[rand() % group.size()]];
							Ped horse = SpawnOneHorse(respawnEntry);
							if (horse != 0)
							{
								sp.ped = horse;
								sp.entry = respawnEntry;
								sp.grounded = false;
								sp.released = false;
							}
						}
						continue;
					}

					if (!isNear && !sp.released)
					{
						Ped tmp = sp.ped;
						ENTITY::SET_PED_AS_NO_LONGER_NEEDED(&tmp);
						sp.released = true;
						continue;
					}

					if (!isNear) continue;

					if (!sp.grounded)
					{
						Vector3 hp = ENTITY::GET_ENTITY_COORDS(sp.ped, TRUE, TRUE);
						STREAMING::REQUEST_COLLISION_AT_COORD(hp.x, hp.y, hp.z);
						float groundZ = 0.0f;
						if (GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(hp.x, hp.y, hp.z + 5.0f, &groundZ, FALSE))
						{
							if (fabsf(hp.z - groundZ) > 0.25f)
								ENTITY::SET_ENTITY_COORDS(sp.ped, hp.x, hp.y, groundZ, FALSE, FALSE, FALSE, FALSE);
							sp.grounded = true;
						}
					}

					if (!sp.genderApplied && sp.entry.gender >= 0)
					{
						sp.genderDelay++;
						if (sp.genderDelay >= 10)
						{
							invoke<Void>(0x5653AB26C82938CF, sp.ped, 0xA28B, (float)(sp.entry.gender == 1 ? 0.0 : 1.0));
							invoke<Void>(0xCC8CA3E88256E58F, sp.ped, false, true, true, false, false);
							sp.genderApplied = true;
						}
					}

					if (sp.grounded && !sp.released)
					{
						Ped tmp = sp.ped;
						ENTITY::SET_PED_AS_NO_LONGER_NEEDED(&tmp);
						sp.released = true;
					}
				}
			}
		}

		// Scanner overlay
		if (g_scanner != 0 && (PLAYER::IS_PLAYER_TARGETTING_ANYTHING(PLAYER::PLAYER_ID()) || PLAYER::IS_PLAYER_FREE_AIMING(PLAYER::PLAYER_ID())))
		{
			Ped playerPed = PLAYER::PLAYER_PED_ID();
			if (ENTITY::DOES_ENTITY_EXIST(playerPed))
			{
				Ped hitPed = 0;
				Hash pedHash = 0;
				Vector3 pedPos = {0,0,0};
				float pedHeading = 0.0f;

				Hash aimHash = 0;
				Vector3 aimPos = {0,0,0};
				float aimHeading = 0.0f;

				// Raycast from camera
				Vector3 camPos = CAM::GET_GAMEPLAY_CAM_COORD();
				Vector3 camRot = CAM::GET_GAMEPLAY_CAM_ROT(2);
				float rz = camRot.z * 0.017453292f;
				float rx = camRot.x * 0.017453292f;
				float fwdX = -sinf(rz) * cosf(rx);
				float fwdY = cosf(rz) * cosf(rx);
				float fwdZ = sinf(rx);
				float rayDist = 50.0f;
				Vector3 endPos = { camPos.x + fwdX * rayDist, camPos.y + fwdY * rayDist, camPos.z + fwdZ * rayDist };
				int rayHandle = SHAPETEST::_START_SHAPE_TEST_RAY(camPos.x, camPos.y, camPos.z, endPos.x, endPos.y, endPos.z, 4294967295, playerPed, 0);
				BOOL rayHit = FALSE;
				Vector3 hitCoords, hitNormal;
				Entity hitEntity;
				SHAPETEST::GET_SHAPE_TEST_RESULT(rayHandle, &rayHit, &hitCoords, &hitNormal, &hitEntity);
				if (rayHit && hitEntity != 0 && ENTITY::DOES_ENTITY_EXIST(hitEntity))
				{
					Vector3 ePos = ENTITY::GET_ENTITY_COORDS(hitEntity, TRUE, TRUE);
					Hash mh = ENTITY::GET_ENTITY_MODEL(hitEntity);
					float hdg = ENTITY::GET_ENTITY_HEADING(hitEntity);
					if (ENTITY::IS_ENTITY_A_PED(hitEntity))
					{
						hitPed = hitEntity;
						pedHash = mh;
						pedPos = ePos;
						pedHeading = hdg;
					}
					else
					{
						aimHash = mh;
						aimPos = ePos;
						aimHeading = hdg;
					}
				}

				// Fallback: look for nearby ped matching bank model hash
				if (aimHash == 0 && g_bankModelHash != 0)
				{
					Vector3 pPos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, TRUE);
					Vector3 foundPos;
					if (FindModelNearby(g_bankModelHash, 5.0f, pPos, foundPos))
					{
						int buf[32 + 1];
						buf[0] = 32;
						int count = PED::GET_PED_NEARBY_PEDS(playerPed, buf, -1, -1);
						for (int i = 0; i < count && i < 32; i++)
						{
							Ped p = (Ped)buf[i + 1];
							if (ENTITY::DOES_ENTITY_EXIST(p) && ENTITY::GET_ENTITY_MODEL(p) == g_bankModelHash)
							{
								aimHash = ENTITY::GET_ENTITY_MODEL(p);
								aimPos = ENTITY::GET_ENTITY_COORDS(p, TRUE, TRUE);
								aimHeading = ENTITY::GET_ENTITY_HEADING(p);
								break;
							}
						}
					}
				}

				// Line 1: ped
				char pedLine[128];
				sprintf_s(pedLine, "0x%08X=X:%.0f;Y:%.0f;Z:%.0f;W:%.0f", pedHash, pedPos.x, pedPos.y, pedPos.z, pedHeading);
				UI::SET_TEXT_SCALE(0.0, 0.35f);
				UI::SET_TEXT_COLOR_RGBA(255, 1, 1, 128);
				UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
				UI::SET_TEXT_CENTRE(1);
				DrawText(0.5f, 0.30f, pedLine);

				int lineY = 33;

				if (hitPed != 0)
				{
					int numComponents = PED::_0x90403E8107B60E81(hitPed);
					if (numComponents > 0 && numComponents < 100)
					{
						DWORD compHashes[64];
						int compCount = 0;
						for (int idx = 0; idx < numComponents && compCount < 64; idx++)
						{
							DWORD out1 = 0, out2 = 0;
							Any shopItemHash = PED::_0x77BA37622E22023B(hitPed, idx, FALSE, &out1, &out2);
							if (shopItemHash != 0)
								compHashes[compCount++] = (DWORD)shopItemHash;
						}

						const char *eqTypeNames[] = { "Saddle", "SaddleBag", "Stirrup", "Horn", "Blanket", "Bedroll" };
						for (int t = 0; t < kApocalypseEquipmentTypeCount; t++)
						{
							for (int c = 0; c < compCount; c++)
							{
								bool matched = false;
								for (size_t h = 0; h < g_apocalypseHorses.size() && !matched; h++)
								{
									const vector<DWORD> &hashes = g_apocalypseHorses[h].equipmentHashesByType[t];
									for (size_t j = 0; j < hashes.size(); j++)
									{
										if (compHashes[c] == hashes[j])
										{
											matched = true;
											break;
										}
									}
								}
								if (matched)
								{
									char eqLine[256];
									sprintf_s(eqLine, "%s: 0x%08X", eqTypeNames[t], compHashes[c]);
									UI::SET_TEXT_SCALE(0.0, 0.35f);
									UI::SET_TEXT_COLOR_RGBA(255, 1, 1, 128);
									UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
									UI::SET_TEXT_CENTRE(1);
									DrawText(0.5f, lineY * 0.01f, eqLine);
									lineY += 3;
									break;
								}
							}
						}
					}
				}

				// Line 2+: container + items
				if (aimHash != 0)
				{
					Hash containerHash = 0;
					Vector3 containerPos = {0,0,0};
					float containerHeading = 0.0f;

					// Find the container: different object near the aimed object
					int nearArr[256];
					int nearCount = worldGetAllObjects(nearArr, 256);
					for (int i = 0; i < nearCount && i < 256; i++)
					{
						Object o = nearArr[i];
						if (!ENTITY::DOES_ENTITY_EXIST(o)) continue;
						Hash mh = ENTITY::GET_ENTITY_MODEL(o);
						if (mh == aimHash) continue;
						Vector3 ePos = ENTITY::GET_ENTITY_COORDS(o, TRUE, TRUE);
						float dx = ePos.x - aimPos.x;
						float dy = ePos.y - aimPos.y;
						float dz = ePos.z - aimPos.z;
						if (dx * dx + dy * dy + dz * dz > 1.0f * 1.0f) continue;
						if (containerHash == 0)
						{
							containerHash = mh;
							containerPos = ePos;
							containerHeading = ENTITY::GET_ENTITY_HEADING(o);
						}
					}

					// Line 2: container (or aimed object if no container found)
					if (containerHash != 0)
					{
						char cLine[128];
						sprintf_s(cLine, "0x%08X=X:%.0f;Y:%.0f;Z:%.0f;W:%.0f", containerHash, containerPos.x, containerPos.y, containerPos.z, containerHeading);
						UI::SET_TEXT_SCALE(0.0, 0.35f);
						UI::SET_TEXT_COLOR_RGBA(255, 1, 1, 128);
						UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
						UI::SET_TEXT_CENTRE(1);
						DrawText(0.5f, lineY * 0.01f, cLine);
						lineY += 3;
					}

					// Lines 3+: aimed object + other items near container
					Vector3 searchPos = containerHash != 0 ? containerPos : aimPos;
					for (int i = 0; i < nearCount && i < 256; i++)
					{
						Object o = nearArr[i];
						if (!ENTITY::DOES_ENTITY_EXIST(o)) continue;
						Hash mh = ENTITY::GET_ENTITY_MODEL(o);
						if (mh == containerHash) continue;
						Vector3 ePos = ENTITY::GET_ENTITY_COORDS(o, TRUE, TRUE);
						float dx = ePos.x - searchPos.x;
						float dy = ePos.y - searchPos.y;
						float dz = ePos.z - searchPos.z;
						if (dx * dx + dy * dy + dz * dz > 1.0f * 1.0f) continue;
						char iLine[128];
						sprintf_s(iLine, "0x%08X=X:%.0f;Y:%.0f;Z:%.0f;W:%.0f", mh, ePos.x, ePos.y, ePos.z, ENTITY::GET_ENTITY_HEADING(o));
						UI::SET_TEXT_SCALE(0.0, 0.35f);
						UI::SET_TEXT_COLOR_RGBA(255, 1, 1, 128);
						UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
						UI::SET_TEXT_CENTRE(1);
						DrawText(0.5f, lineY * 0.01f, iLine);
						lineY += 3;
					}
				}
			}
		}
		
		// FPS overlay
		if (g_fps != 0)
		{
			static DWORD fpsLastTick = GetTickCount();
			static int fpsFrames = 0;
			static float fpsValue = 0.0f;
			DWORD now = GetTickCount();
			fpsFrames++;
			if (now - fpsLastTick >= 500)
			{
				fpsValue = (float)fpsFrames * 1000.0f / (float)(now - fpsLastTick);
				fpsFrames = 0;
				fpsLastTick = now;
			}
			char fpsBuf[32];
			sprintf_s(fpsBuf, "FPS: %.0f", fpsValue);
			UI::SET_TEXT_SCALE(0.0, 0.24f);
			if (g_fps >= 2)
			{
				if (fpsValue >= 60.0f)      UI::SET_TEXT_COLOR_RGBA(255, 255, 255, 255);
				else if (fpsValue >= 30.0f) UI::SET_TEXT_COLOR_RGBA(255, 255, 0, 255);
				else                        UI::SET_TEXT_COLOR_RGBA(255, 0, 0, 255);
			}
			else
			{
				UI::SET_TEXT_COLOR_RGBA(255, 255, 255, 255);
			}
			UI::SET_TEXT_DROPSHADOW(2, 0, 0, 0, 0);
			UI::SET_TEXT_CENTRE(0);
			DrawText(0.01f, 0.01f, fpsBuf);
		}

		// GPS coords overlay
		if (g_gps != 0)
		{
			Ped playerPed = PLAYER::PLAYER_PED_ID();
			if (ENTITY::DOES_ENTITY_EXIST(playerPed))
			{
				Vector3 pos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, TRUE);
				char gpsBuf[64];
				if (g_gps >= 2)
					sprintf_s(gpsBuf, "X:%.0f,Y:%.0f,Z:%.0f,W:%.0f", pos.x, pos.y, pos.z, ENTITY::GET_ENTITY_HEADING(playerPed));
				else
					sprintf_s(gpsBuf, "X:%.0f,Y:%.0f,Z:%.0f", pos.x, pos.y, pos.z);
				UI::SET_TEXT_SCALE(0.0, 0.24f);
				UI::SET_TEXT_COLOR_RGBA(255, 255, 255, 200);
				UI::SET_TEXT_DROPSHADOW(2, 0, 0, 0, 0);
				UI::SET_TEXT_CENTRE(0);
				DrawText(0.01f, 0.98f, gpsBuf);
			}
		}

		// KMH speed monitor (bottom right, only when mounted)
		if (g_kmh != 0)
		{
			Ped playerPed = PLAYER::PLAYER_PED_ID();
			if (ENTITY::DOES_ENTITY_EXIST(playerPed))
			{
				Ped mount = PED::GET_MOUNT(playerPed);
				if (mount != 0)
				{
					float speedMs = ENTITY::GET_ENTITY_SPEED(mount);
					float speedKmh = speedMs * 3.6f;
					char kmhBuf[32];
					sprintf_s(kmhBuf, "Speed: %.0f km/h", speedKmh);
					float textWidth = strlen(kmhBuf) * 0.005f;
					UI::SET_TEXT_SCALE(0.0, 0.24f);
					UI::SET_TEXT_COLOR_RGBA(255, 255, 255, 200);
					UI::SET_TEXT_DROPSHADOW(2, 0, 0, 0, 0);
					UI::SET_TEXT_CENTRE(0);
					DrawText(0.99f - textWidth, 0.98f, kmhBuf);
				}
			}
		}

		// Clear All Bounties and Lockdown Areas (Arthur with Bandana)
		if (g_clear_All_Bounties_and_Lockdown_Areas != 0)
		{
			Ped playerPed = PLAYER::PLAYER_PED_ID();
			Player player = PLAYER::PLAYER_ID();
			if (ENTITY::DOES_ENTITY_EXIST(playerPed) && PLAYER::IS_PLAYER_PLAYING(player))
			{
				bool hasBandana = false;
				int numCategories = PED::_GET_NUM_COMPONENT_CATEGORIES_IN_PED(playerPed);
				if (numCategories > 0 && numCategories < 100)
				{
					for (int i = 0; i < numCategories; i++)
					{
						Any cat = PED::_GET_PED_COMPONENT_CATEGORY_BY_INDEX(playerPed, i);
						if (cat == 0x4A9DA893 || cat == 0x733849E4 || cat == 0x9C5D9C90)
						{
							hasBandana = true;
							break;
						}
					}
				}

				SniperHook_SetDisguised(hasBandana);

				if (hasBandana)
				{
					PLAYER::SET_MAX_WANTED_LEVEL(0);
					PLAYER::RESET_WANTED_LEVEL_DIFFICULTY(player);
					MAPREGION::_0xBE551C2CC421185D(0x41759831, FALSE);
					MAPREGION::_0xBE551C2CC421185D(0xD69B5B49, FALSE);
					MAPREGION::_0xBE551C2CC421185D(0x5647E155, FALSE);
					MAPREGION::_0xBE551C2CC421185D(0x129E1411, FALSE);
					MAPREGION::_0xBE551C2CC421185D(0x0E95FF51, FALSE);
					MAPREGION::_0xBE551C2CC421185D(0x763A8A87, FALSE);
					MAPREGION::_0xBE551C2CC421185D(0x33D88587, FALSE);
					MAPREGION::_0xBE551C2CC421185D(0x99B6A1E6, FALSE);
					MAPREGION::_0xBE551C2CC421185D(0x8966022D, FALSE);
					MAPREGION::_0xBE551C2CC421185D(0x3AC128F9, FALSE);
					MAPREGION::_0xBE551C2CC421185D(0x27253ED3, FALSE);
					MAPREGION::_0xBE551C2CC421185D(0x5046DD11, FALSE);
					MAPREGION::_0xBE551C2CC421185D(0xD428627B, FALSE);
					MAPREGION::_0xBE551C2CC421185D(0xFAF570C5, FALSE);
				}
				else
				{
					PLAYER::SET_MAX_WANTED_LEVEL(5);
					PLAYER::SET_WANTED_LEVEL_MULTIPLIER(1.0f);
					MAPREGION::_0xBE551C2CC421185D(0x41759831, TRUE);
					MAPREGION::_0xBE551C2CC421185D(0xD69B5B49, TRUE);
					MAPREGION::_0xBE551C2CC421185D(0x5647E155, TRUE);
					MAPREGION::_0xBE551C2CC421185D(0x129E1411, TRUE);
					MAPREGION::_0xBE551C2CC421185D(0x0E95FF51, TRUE);
					MAPREGION::_0xBE551C2CC421185D(0x763A8A87, TRUE);
					MAPREGION::_0xBE551C2CC421185D(0x33D88587, TRUE);
					MAPREGION::_0xBE551C2CC421185D(0x99B6A1E6, TRUE);
					MAPREGION::_0xBE551C2CC421185D(0x8966022D, TRUE);
					MAPREGION::_0xBE551C2CC421185D(0x3AC128F9, TRUE);
					MAPREGION::_0xBE551C2CC421185D(0x27253ED3, TRUE);
					MAPREGION::_0xBE551C2CC421185D(0x5046DD11, TRUE);
					MAPREGION::_0xBE551C2CC421185D(0xD428627B, TRUE);
					MAPREGION::_0xBE551C2CC421185D(0xFAF570C5, TRUE);
				}
			}
		}
		else
		{
			SniperHook_SetDisguised(false);
		}

		// Fog of War
		if (g_fog_of_War != 0)
			RADAR::_SET_MINIMAP_REVEALED(TRUE);

		// Perfect Horses: apply or restore stats on the player's mount and nearby mod-spawned horses.
		// This always runs so a boost is removed immediately after changing equipment.
		{
			Ped playerPed = PLAYER::PLAYER_PED_ID();
			if (ENTITY::DOES_ENTITY_EXIST(playerPed))
			{
				// Apply to player's current mount
				Ped mount = PED::GET_MOUNT(playerPed);
				if (mount != 0 && ENTITY::DOES_ENTITY_EXIST(mount) && !ENTITY::IS_ENTITY_DEAD(mount))
				{
					ApplyPerfectHorseStats(mount);
				}

				// Apply to nearby mod-spawned horses
				Vector3 ppos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, TRUE);
				for (size_t i = 0; i < g_spawnedPeds.size(); i++)
				{
					SpawnedPed &sp = g_spawnedPeds[i];
					if (sp.ped != mount && ENTITY::DOES_ENTITY_EXIST(sp.ped) && !ENTITY::IS_ENTITY_DEAD(sp.ped))
					{
						float dx = sp.x - ppos.x;
						float dy = sp.y - ppos.y;
						if (dx * dx + dy * dy <= 500.0f * 500.0f)
						{
							ApplyPerfectHorseStats(sp.ped);
						}
					}
				}
			}
		}

		// Bank overlay
		if (!g_bankPhrases.empty())
		{
			if (g_bankState == GB_PROMPT)
			{
				UI::SET_TEXT_SCALE(0.0, 0.32f);
				UI::SET_TEXT_COLOR_RGBA(255, 0, 0, 64);
				UI::SET_TEXT_DROPSHADOW(2, 0, 0, 0, 0);
				UI::SET_TEXT_CENTRE(1);
				DrawText(0.5f, 0.90f, "???");
			}
			else if (g_bankState == GB_KEYBOARD)
			{
				GRAPHICS::DRAW_RECT(0.5f, 0.295f, 0.40f, 0.05f, 200, 20, 20, 200, FALSE, FALSE);
				GRAPHICS::DRAW_RECT(0.5f, 0.345f, 0.40f, 0.05f, 20, 20, 20, 200, FALSE, FALSE);

				UI::SET_TEXT_SCALE(0.0, 0.32f);
				UI::SET_TEXT_COLOR_RGBA(255, 255, 255, 255);
				UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
				UI::SET_TEXT_CENTRE(1);
				DrawText(0.5f, 0.29f, "ENTER CHEAT");

				char kbLine[256];
				sprintf_s(kbLine, "%s_", g_kbBuffer);
				UI::SET_TEXT_SCALE(0.0, 0.32f);
				UI::SET_TEXT_COLOR_RGBA(255, 255, 255, 255);
				UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
				UI::SET_TEXT_CENTRE(1);
				DrawText(0.5f, 0.34f, kbLine);
			}
			else if (g_bankState == GB_SUCCESS)
			{
				UI::SET_TEXT_SCALE(0.0, 0.32f);
				UI::SET_TEXT_COLOR_RGBA(0, 255, 0, 255);
				UI::SET_TEXT_DROPSHADOW(2, 0, 0, 0, 0);
				UI::SET_TEXT_CENTRE(1);
				DrawText(0.5f, 0.90f, "$500");
			}
			else if (g_bankState == GB_ROBBERY)
			{
				UI::SET_TEXT_SCALE(0.0, 0.32f);
				UI::SET_TEXT_COLOR_RGBA(255, 0, 0, 255);
				UI::SET_TEXT_DROPSHADOW(2, 0, 0, 0, 0);
				UI::SET_TEXT_CENTRE(1);
				DrawText(0.5f, 0.90f, "!!!");
			}
		}

		// Outfit unlock overlay
		if (!g_outfitPhrases.empty())
		{
			if (g_outfitState == OUTFITS_PROMPT)
			{
				UI::SET_TEXT_SCALE(0.0, 0.32f);
				UI::SET_TEXT_COLOR_RGBA(255, 0, 0, 64);
				UI::SET_TEXT_DROPSHADOW(2, 0, 0, 0, 0);
				UI::SET_TEXT_CENTRE(1);
				DrawText(0.5f, 0.90f, "???");
			}
			else if (g_outfitState == OUTFITS_KEYBOARD)
			{
				GRAPHICS::DRAW_RECT(0.5f, 0.295f, 0.40f, 0.05f, 200, 20, 20, 200, FALSE, FALSE);
				GRAPHICS::DRAW_RECT(0.5f, 0.345f, 0.40f, 0.05f, 20, 20, 20, 200, FALSE, FALSE);

				UI::SET_TEXT_SCALE(0.0, 0.32f);
				UI::SET_TEXT_COLOR_RGBA(255, 255, 255, 255);
				UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
				UI::SET_TEXT_CENTRE(1);
				DrawText(0.5f, 0.29f, "ENTER CHEAT");

				char kbLine[256];
				sprintf_s(kbLine, "%s_", g_kbBuffer);
				UI::SET_TEXT_SCALE(0.0, 0.32f);
				UI::SET_TEXT_COLOR_RGBA(255, 255, 255, 255);
				UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
				UI::SET_TEXT_CENTRE(1);
				DrawText(0.5f, 0.34f, kbLine);
			}
			else if (g_outfitState == OUTFITS_SUCCESS)
			{
				UI::SET_TEXT_SCALE(0.0, 0.32f);
				UI::SET_TEXT_COLOR_RGBA(0, 255, 0, 255);
				UI::SET_TEXT_DROPSHADOW(2, 0, 0, 0, 0);
				UI::SET_TEXT_CENTRE(1);
				DrawText(0.5f, 0.90f, "!!!");
			}
		}

		menuController->Update();
		WAIT(0);
	}
}

void ScriptMain()
{
	srand(GetTickCount());
	main();
}

// ==

struct scrVector {
	float x, y, z, _pad;
};

struct scrVec3N {
	float x, y, z;
};

class scrNativeCallContext {
public:
	void* m_ReturnValue;       // 0x0000
	uint32_t m_ArgCount;       // 0x0008
	uint64_t* m_Args;          // 0x0010
	uint32_t m_DataCount;      // 0x0018
	uint32_t pad2;             // 0x001C
	scrVector* m_OutVectors[4];// 0x0020
	scrVec3N m_InVectors[4];   // 0x0040
	uint8_t _pad[96];          // 0x0070

	template <typename T>
	T GetArg(int Index) const
	{
		return *(T*)&m_Args[Index];
	}
};

using scrNativeHandler = void(*)(scrNativeCallContext*);

static constexpr uint32_t joaat_hash(const char* String)
{
	uint32_t Hash = 0;
	while (*String)
	{
		char c = *String++;
		if (c >= 'A' && c <= 'Z')
			c += ('a' - 'A');
		Hash += c;
		Hash += (Hash << 10);
		Hash ^= (Hash >> 6);
	}
	Hash += (Hash << 3);
	Hash ^= (Hash >> 11);
	Hash += (Hash << 15);
	return Hash;
}

struct PatternByte {
	uint8_t value;
	bool wildcard;
};

static bool ParsePattern(const char* str, PatternByte* out, int* outCount)
{
	int count = 0;
	while (*str && count < 128)
	{
		while (*str == ' ') str++;
		if (!*str) break;

		if (*str == '?')
		{
			out[count].wildcard = true;
			out[count].value = 0;
			count++;
			if (str[1] == '?') str++;
			str++;
		}
		else
		{
			auto hex = [](char c) -> uint8_t {
				if (c >= '0' && c <= '9') return c - '0';
				if (c >= 'a' && c <= 'f') return c - 'a' + 10;
				if (c >= 'A' && c <= 'F') return c - 'A' + 10;
				return 0;
			};
			out[count].wildcard = false;
			out[count].value = (hex(str[0]) << 4) | hex(str[1]);
			count++;
			str += 2;
		}
	}
	*outCount = count;
	return count > 0;
}

static uintptr_t PatternScan(HMODULE module, const char* patternStr)
{
	MODULEINFO info{};
	if (!GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(info)))
		return 0;

	PatternByte pattern[128];
	int patLen = 0;
	ParsePattern(patternStr, pattern, &patLen);
	if (patLen == 0) return 0;

	const uint8_t* start = (const uint8_t*)info.lpBaseOfDll;
	const uint8_t* end = start + info.SizeOfImage - patLen;

	for (const uint8_t* p = start; p < end; ++p)
	{
		bool match = true;
		for (int i = 0; i < patLen; ++i)
		{
			if (!pattern[i].wildcard && p[i] != pattern[i].value)
			{
				match = false;
				break;
			}
		}
		if (match)
			return (uintptr_t)p;
	}
	return 0;
}

static constexpr int HOOK_SIZE = 14;

struct NativeHook {
	uintptr_t target;
	uint8_t originalBytes[HOOK_SIZE];
	uintptr_t detour;
	DWORD originalProtect;
	bool active;
};

static void ApplyJump(uint8_t* dst, uintptr_t target)
{
	dst[0] = 0xFF;
	dst[1] = 0x25;
	*(uint32_t*)(dst + 2) = 0;
	*(uint64_t*)(dst + 6) = target;
}

static bool HookInstall(NativeHook* hook, uintptr_t target, uintptr_t detour)
{
	hook->target = target;
	hook->detour = detour;
	hook->active = false;
	hook->originalProtect = 0;

	memcpy(hook->originalBytes, (void*)target, HOOK_SIZE);

	DWORD oldProtect;
	if (!VirtualProtect((void*)target, HOOK_SIZE, PAGE_EXECUTE_READWRITE, &oldProtect))
		return false;
	hook->originalProtect = oldProtect;

	ApplyJump((uint8_t*)target, detour);

	VirtualProtect((void*)target, HOOK_SIZE, oldProtect, &oldProtect);
	hook->active = true;
	return true;
}

static void HookRemove(NativeHook* hook)
{
	if (!hook->active) return;

	DWORD oldProtect;
	VirtualProtect((void*)hook->target, HOOK_SIZE, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy((void*)hook->target, hook->originalBytes, HOOK_SIZE);
	VirtualProtect((void*)hook->target, HOOK_SIZE, hook->originalProtect, &oldProtect);

	hook->active = false;
}

static void HookCallOriginal(NativeHook* hook, scrNativeCallContext* ctx)
{
	DWORD oldProtect;
	VirtualProtect((void*)hook->target, HOOK_SIZE, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy((void*)hook->target, hook->originalBytes, HOOK_SIZE);
	VirtualProtect((void*)hook->target, HOOK_SIZE, hook->originalProtect, &oldProtect);

	using Fn = void(*)(scrNativeCallContext*);
	((Fn)hook->target)(ctx);

	VirtualProtect((void*)hook->target, HOOK_SIZE, PAGE_EXECUTE_READWRITE, &oldProtect);
	ApplyJump((uint8_t*)hook->target, hook->detour);
	VirtualProtect((void*)hook->target, HOOK_SIZE, hook->originalProtect, &oldProtect);
}

using GetNativeHandlerFn = scrNativeHandler(*)(uint64_t hash);

static GetNativeHandlerFn g_GetNativeHandler = nullptr;

static scrNativeHandler QueryNativeHandler(uint64_t hash)
{
	if (!g_GetNativeHandler) return nullptr;
	return g_GetNativeHandler(hash);
}

static NativeHook g_shootHook{};
static NativeHook g_angledAreaHook{};
static bool g_disguised = false;
static bool g_hooksActive = false;

static constexpr uint32_t WEAPON_SNIPERRIFLE_CARCANO_HASH = joaat_hash("WEAPON_SNIPERRIFLE_CARCANO");
static constexpr uint32_t GUARMA_ORIGIN_X_BITS = 0x44BBD654;

static void Detour_ShootSingleBullet(scrNativeCallContext* ctx)
{
	if (g_disguised && ctx && ctx->GetArg<uint32_t>(8) == WEAPON_SNIPERRIFLE_CARCANO_HASH)
	{
		return;
	}

	HookCallOriginal(&g_shootHook, ctx);
}

static void Detour_IsEntityInAngledArea(scrNativeCallContext* ctx)
{
	if (g_disguised && ctx && ctx->GetArg<uint32_t>(1) == GUARMA_ORIGIN_X_BITS)
	{
		ctx->m_ReturnValue = (void*)0;
		return;
	}

	HookCallOriginal(&g_angledAreaHook, ctx);
}

static void SniperHook_SetDisguised(bool disguised)
{
	g_disguised = disguised;
}

static bool SniperHook_IsActive()
{
	return g_hooksActive;
}

static bool SniperHook_Initialize()
{
	HMODULE gameModule = GetModuleHandle(NULL);
	if (!gameModule)
		return false;

	uintptr_t callSite = PatternScan(gameModule, "E8 ?? ?? ?? ?? 42 8B 9C FE");
	if (!callSite)
		return false;

	int32_t relOffset = *(int32_t*)(callSite + 1);
	uintptr_t getNativeHandlerAddr = callSite + 5 + relOffset;

	g_GetNativeHandler = (GetNativeHandlerFn)getNativeHandlerAddr;
	if (!g_GetNativeHandler)
		return false;

	scrNativeHandler shootHandler = QueryNativeHandler(0x867654CBC7606F2C);
	scrNativeHandler angledAreaHandler = QueryNativeHandler(0xD3151E53134595E5);

	if (!shootHandler || !angledAreaHandler)
		return false;

	if (!HookInstall(&g_shootHook, (uintptr_t)shootHandler, (uintptr_t)Detour_ShootSingleBullet))
		return false;

	if (!HookInstall(&g_angledAreaHook, (uintptr_t)angledAreaHandler, (uintptr_t)Detour_IsEntityInAngledArea))
		return false;

	g_hooksActive = true;
	return true;
}

void SniperHook_Shutdown()
{
	g_hooksActive = false;
	HookRemove(&g_shootHook);
	HookRemove(&g_angledAreaHook);
	g_GetNativeHandler = nullptr;
}
