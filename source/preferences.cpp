/****************************************************************************
 * Snes9x Nintendo Wii/GameCube Port
 *
 * Tantric 2008-2022
 * Tanooki 2019-2023
 *
 * preferences.cpp
 *
 * Preferences save/load to XML file
 ***************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ogcsys.h>
#include <mxml.h>

#include "snes9xtx.h"
#include "menu.h"
#include "fileop.h"
#include "video.h"
#include "filebrowser.h"
#include "input.h"
#include "button_mapping.h"

#include "snes9x/apu/apu.h"

struct SGCSettings GCSettings;

/****************************************************************************
 * Prepare Preferences Data
 *
 * This sets up the save buffer for saving.
 ***************************************************************************/
static mxml_node_t *xml = NULL;
static mxml_node_t *data = NULL;
static mxml_node_t *section = NULL;
static mxml_node_t *item = NULL;
static mxml_node_t *elem = NULL;

static char temp[200];

static const char * toStr(int i)
{
	sprintf(temp, "%d", i);
	return temp;
}

static const char * FtoStr(float i)
{
	sprintf(temp, "%.2f", i);
	return temp;
}

static void createXMLSection(const char * name, const char * description)
{
	section = mxmlNewElement(data, "section");
	mxmlElementSetAttr(section, "name", name);
	mxmlElementSetAttr(section, "description", description);
}

static void createXMLSetting(const char * name, const char * description, const char * value)
{
	item = mxmlNewElement(section, "setting");
	mxmlElementSetAttr(item, "name", name);
	mxmlElementSetAttr(item, "value", value);
	mxmlElementSetAttr(item, "description", description);
}

static void createXMLController(u32 controller[], const char * name, const char * description)
{
	item = mxmlNewElement(section, "controller");
	mxmlElementSetAttr(item, "name", name);
	mxmlElementSetAttr(item, "description", description);

	// create buttons
	for(int i=0; i < MAXJP; i++)
	{
		elem = mxmlNewElement(item, "button");
		mxmlElementSetAttr(elem, "number", toStr(i));
		mxmlElementSetAttr(elem, "assignment", toStr(controller[i]));
	}
}

static const char * XMLSaveCallback(mxml_node_t *node, int where)
{
	const char *name;

	name = node->value.element.name;

	if(where == MXML_WS_BEFORE_CLOSE)
	{
		if(!strcmp(name, "file") || !strcmp(name, "section"))
			return ("\n");
		else if(!strcmp(name, "controller"))
			return ("\n\t");
	}
	if (where == MXML_WS_BEFORE_OPEN)
	{
		if(!strcmp(name, "file"))
			return ("\n");
		else if(!strcmp(name, "section"))
			return ("\n\n");
		else if(!strcmp(name, "setting") || !strcmp(name, "controller"))
			return ("\n\t");
		else if(!strcmp(name, "button"))
			return ("\n\t\t");
	}
	return (NULL);
}

static int
preparePrefsData ()
{
	xml = mxmlNewXML("1.0");
	mxmlSetWrapMargin(0); // disable line wrapping

	data = mxmlNewElement(xml, "file");
	mxmlElementSetAttr(data, "app", APPNAME);
	mxmlElementSetAttr(data, "version", APPVERSION);

	createXMLSection("File", "File Settings");

	createXMLSetting("AutoLoad", "Auto Load", toStr(GCSettings.AutoLoad));
	createXMLSetting("AutoSave", "Auto Save", toStr(GCSettings.AutoSave));
	createXMLSetting("LoadMethod", "Load Method", toStr(GCSettings.LoadMethod));
	createXMLSetting("SaveMethod", "Save Method", toStr(GCSettings.SaveMethod));
	createXMLSetting("LoadFolder", "Load Folder", GCSettings.LoadFolder);
	createXMLSetting("LastFileLoaded", "Last File Loaded", GCSettings.LastFileLoaded);
	createXMLSetting("SaveFolder", "Save Folder", GCSettings.SaveFolder);
	createXMLSetting("CheatFolder", "Cheats Folder", GCSettings.CheatFolder);
	createXMLSetting("ScreenshotsFolder", "Screenshots Folder", GCSettings.ScreenshotsFolder);
	createXMLSetting("CoverFolder", "Covers Folder", GCSettings.CoverFolder);
	createXMLSetting("ArtworkFolder", "Artwork Folder", GCSettings.ArtworkFolder);

	createXMLSection("Video", "Video Settings");

	createXMLSetting("videomode", "Video Mode", toStr(GCSettings.videomode));
	createXMLSetting("zoomHor", "Horizontal Zoom Level", FtoStr(GCSettings.zoomHor));
	createXMLSetting("zoomVert", "Vertical Zoom Level", FtoStr(GCSettings.zoomVert));
	createXMLSetting("render", "Rendering", toStr(GCSettings.render));
	createXMLSetting("bilinear", "Bilinear Filtering", toStr(GCSettings.bilinear));
	createXMLSetting("aspect", "Aspect Ratio", toStr(GCSettings.aspect));
	createXMLSetting("VideoFilter", "Video Filter", toStr(GCSettings.VideoFilter));
	createXMLSetting("HiResMode", "Hi-Res Mode", toStr(GCSettings.HiResMode));
	createXMLSetting("FrameSkip", "Frame Skipping", toStr(GCSettings.FrameSkip));
	createXMLSetting("ShowFrameRate", "Show Frame Rate", toStr(GCSettings.ShowFrameRate));
	createXMLSetting("crosshair", "Show Crosshair", toStr(GCSettings.crosshair));
	createXMLSetting("xshift", "Horizontal Video Shift", toStr(GCSettings.xshift));
	createXMLSetting("yshift", "Vertical Video Shift", toStr(GCSettings.yshift));

	createXMLSection("Audio", "Audio Settings");

	createXMLSetting("MuteSound", "Mute Sound", toStr(GCSettings.MuteSound));
	createXMLSetting("Interpolation", "Sound Interpolation", toStr(GCSettings.Interpolation));

	createXMLSection("Emulation Hacks", "Emulation Hacks Settings");

	createXMLSetting("sfxOverclock", "SuperFX Overclocking", toStr(GCSettings.sfxOverclock));
	createXMLSetting("cpuOverclock", "CPU Overclocking", toStr(GCSettings.cpuOverclock));
	createXMLSetting("NoSpriteLimit", "No Sprite Limit", toStr(GCSettings.NoSpriteLimit));

	createXMLSection("Emulation", "Emulation Settings");

	createXMLSetting("Satellaview", "Satellaview BIOS", toStr(GCSettings.Satellaview));

	createXMLSection("Menu", "Menu Settings");

#ifdef HW_RVL
	createXMLSetting("WiimoteOrientation", "Wiimote Orientation", toStr(GCSettings.WiimoteOrientation));
#endif
	createXMLSetting("ExitAction", "Exit Action", toStr(GCSettings.ExitAction));
	createXMLSetting("MusicVolume", "Music Volume", toStr(GCSettings.MusicVolume));
	createXMLSetting("SFXVolume", "Sound Effects Volume", toStr(GCSettings.SFXVolume));
	createXMLSetting("language", "Language", toStr(GCSettings.language));
	createXMLSetting("PreviewImage", "Preview Image", toStr(GCSettings.PreviewImage));
	createXMLSetting("HideSRAMSaving", "Hide SRAM Saving", toStr(GCSettings.HideSRAMSaving));	

	createXMLSection("Controller", "Controller Settings");

	createXMLSetting("Controller", "Controller", toStr(GCSettings.Controller));
	createXMLSetting("FastForward", "Fast Forward", toStr(GCSettings.FastForward));
	createXMLSetting("FastForwardButton", "Fast Forward Button", toStr(GCSettings.FastForwardButton));

	createXMLController(btnmap[CTRL_PAD][CTRLR_GCPAD], "btnmap_pad_gcpad", "SNES Pad - GameCube Controller");
#ifdef HW_RVL
	createXMLController(btnmap[CTRL_PAD][CTRLR_WIIMOTE], "btnmap_pad_wiimote", "SNES Pad - Wiimote");
	createXMLController(btnmap[CTRL_PAD][CTRLR_CLASSIC], "btnmap_pad_classic", "SNES Pad - Classic Controller");
	createXMLController(btnmap[CTRL_PAD][CTRLR_WUPC], "btnmap_pad_wupc", "SNES Pad - Wii U Pro Controller");
	createXMLController(btnmap[CTRL_PAD][CTRLR_WIIDRC], "btnmap_pad_wiidrc", "SNES Pad - Wii U Gamepad");
	createXMLController(btnmap[CTRL_PAD][CTRLR_NUNCHUK], "btnmap_pad_nunchuk", "SNES Pad - Nunchuk + Wiimote");
#endif
	createXMLController(btnmap[CTRL_SCOPE][CTRLR_GCPAD], "btnmap_scope_gcpad", "Superscope - GameCube Controller");
#ifdef HW_RVL
	createXMLController(btnmap[CTRL_SCOPE][CTRLR_WIIMOTE], "btnmap_scope_wiimote", "Superscope - Wiimote");
#endif
	createXMLController(btnmap[CTRL_MOUSE][CTRLR_GCPAD], "btnmap_mouse_gcpad", "Mouse - GameCube Controller");
#ifdef HW_RVL
	createXMLController(btnmap[CTRL_MOUSE][CTRLR_WIIMOTE], "btnmap_mouse_wiimote", "Mouse - Wiimote");
#endif
	createXMLController(btnmap[CTRL_JUST][CTRLR_GCPAD], "btnmap_just_gcpad", "Justifier - GameCube Controller");
#ifdef HW_RVL
	createXMLController(btnmap[CTRL_JUST][CTRLR_WIIMOTE], "btnmap_just_wiimote", "Justifier - Wiimote");
#endif
	int datasize = mxmlSaveString(xml, (char *)savebuffer, SAVEBUFFERSIZE, XMLSaveCallback);

	mxmlDelete(xml);

	return datasize;
}

/****************************************************************************
 * loadXMLSetting
 *
 * Load XML elements into variables for an individual variable
 ***************************************************************************/
static void loadXMLSetting(char * var, const char * name, int maxsize)
{
	item = mxmlFindElement(xml, xml, "setting", "name", name, MXML_DESCEND);
	if(item)
	{
		const char * tmp = mxmlElementGetAttr(item, "value");
		if(tmp)
			snprintf(var, maxsize, "%s", tmp);
	}
}
static void loadXMLSetting(int * var, const char * name)
{
	item = mxmlFindElement(xml, xml, "setting", "name", name, MXML_DESCEND);
	if(item)
	{
		const char * tmp = mxmlElementGetAttr(item, "value");
		if(tmp)
			*var = atoi(tmp);
	}
}
static void loadXMLSetting(float * var, const char * name)
{
	item = mxmlFindElement(xml, xml, "setting", "name", name, MXML_DESCEND);
	if(item)
	{
		const char * tmp = mxmlElementGetAttr(item, "value");
		if(tmp)
			*var = atof(tmp);
	}
}

/****************************************************************************
 * loadXMLController
 *
 * Load XML elements into variables for a controller mapping
 ***************************************************************************/
static void loadXMLController(u32 controller[], const char * name)
{
	item = mxmlFindElement(xml, xml, "controller", "name", name, MXML_DESCEND);

	if(item)
	{
		// populate buttons
		for(int i=0; i < MAXJP; i++)
		{
			elem = mxmlFindElement(item, xml, "button", "number", toStr(i), MXML_DESCEND);
			if(elem)
			{
				const char * tmp = mxmlElementGetAttr(elem, "assignment");
				if(tmp)
					controller[i] = atoi(tmp);
			}
		}
	}
}

/****************************************************************************
 * decodePrefsData
 *
 * Decodes preferences - parses XML and loads preferences into the variables
 ***************************************************************************/
static bool
decodePrefsData ()
{
	bool result = false;

	xml = mxmlLoadString(NULL, (char *)savebuffer, MXML_TEXT_CALLBACK);

	if(xml)
	{
		// check settings version
		item = mxmlFindElement(xml, xml, "file", "version", NULL, MXML_DESCEND);
		if(item) // a version entry exists
		{
			const char * version = mxmlElementGetAttr(item, "version");

			if(version && strlen(version) == 5)
			{
				// this code assumes version in format X.X.X
				// XX.X.X, X.XX.X, or X.X.XX will NOT work
				int verMajor = version[0] - '0';
				int verMinor = version[2] - '0';
				int verPoint = version[4] - '0';

				// check that the versioning is valid
				if(!(verMajor >= 1 && verMajor <= 9 &&
					verMinor >= 0 && verMinor <= 9 &&
					verPoint >= 0 && verPoint <= 9)) {
					result = false;
				}
				else {
					result = true;
				}
			}
		}

		if(result)
		{
			// File Settings

			loadXMLSetting(&GCSettings.AutoLoad, "AutoLoad");
			loadXMLSetting(&GCSettings.AutoSave, "AutoSave");
			loadXMLSetting(&GCSettings.LoadMethod, "LoadMethod");
			loadXMLSetting(&GCSettings.SaveMethod, "SaveMethod");
			loadXMLSetting(GCSettings.LoadFolder, "LoadFolder", sizeof(GCSettings.LoadFolder));
			loadXMLSetting(GCSettings.LastFileLoaded, "LastFileLoaded", sizeof(GCSettings.LastFileLoaded));
			loadXMLSetting(GCSettings.SaveFolder, "SaveFolder", sizeof(GCSettings.SaveFolder));
			loadXMLSetting(GCSettings.CheatFolder, "CheatFolder", sizeof(GCSettings.CheatFolder));
			loadXMLSetting(GCSettings.ScreenshotsFolder, "ScreenshotsFolder", sizeof(GCSettings.ScreenshotsFolder));
			loadXMLSetting(GCSettings.CoverFolder, "CoverFolder", sizeof(GCSettings.CoverFolder));
			loadXMLSetting(GCSettings.ArtworkFolder, "ArtworkFolder", sizeof(GCSettings.ArtworkFolder));

			// Video Settings

			loadXMLSetting(&GCSettings.videomode, "videomode");
			loadXMLSetting(&GCSettings.zoomHor, "zoomHor");
			loadXMLSetting(&GCSettings.zoomVert, "zoomVert");
			loadXMLSetting(&GCSettings.render, "render");
			loadXMLSetting(&GCSettings.bilinear, "bilinear");
			loadXMLSetting(&GCSettings.aspect, "aspect");
			loadXMLSetting(&GCSettings.VideoFilter, "VideoFilter");
			loadXMLSetting(&GCSettings.HiResMode, "HiResMode");
			loadXMLSetting(&GCSettings.FrameSkip, "FrameSkip");
			loadXMLSetting(&GCSettings.ShowFrameRate, "ShowFrameRate");
			loadXMLSetting(&GCSettings.crosshair, "crosshair");
			loadXMLSetting(&GCSettings.xshift, "xshift");
			loadXMLSetting(&GCSettings.yshift, "yshift");

			// Audio Settings

			loadXMLSetting(&GCSettings.MuteSound, "MuteSound");
			loadXMLSetting(&GCSettings.Interpolation, "Interpolation");

			// Emulation Hacks Settings

			loadXMLSetting(&GCSettings.sfxOverclock, "sfxOverclock");
			loadXMLSetting(&GCSettings.cpuOverclock, "cpuOverclock");
			loadXMLSetting(&GCSettings.NoSpriteLimit, "NoSpriteLimit");

			// Emulation Settings

			loadXMLSetting(&GCSettings.Satellaview, "Satellaview");

			// Menu Settings

			loadXMLSetting(&GCSettings.WiimoteOrientation, "WiimoteOrientation");
			loadXMLSetting(&GCSettings.ExitAction, "ExitAction");
			loadXMLSetting(&GCSettings.MusicVolume, "MusicVolume");
			loadXMLSetting(&GCSettings.SFXVolume, "SFXVolume");
			loadXMLSetting(&GCSettings.language, "language");
			loadXMLSetting(&GCSettings.PreviewImage, "PreviewImage");
			loadXMLSetting(&GCSettings.HideSRAMSaving, "HideSRAMSaving");

			// Controller Settings

			loadXMLSetting(&GCSettings.Controller, "Controller");
			loadXMLSetting(&GCSettings.FastForward, "FastForward");
			loadXMLSetting(&GCSettings.FastForwardButton, "FastForwardButton");

			loadXMLController(btnmap[CTRL_PAD][CTRLR_GCPAD], "btnmap_pad_gcpad");
			loadXMLController(btnmap[CTRL_PAD][CTRLR_WIIMOTE], "btnmap_pad_wiimote");
			loadXMLController(btnmap[CTRL_PAD][CTRLR_CLASSIC], "btnmap_pad_classic");
			loadXMLController(btnmap[CTRL_PAD][CTRLR_WUPC], "btnmap_pad_wupc");
			loadXMLController(btnmap[CTRL_PAD][CTRLR_WIIDRC], "btnmap_pad_wiidrc");
			loadXMLController(btnmap[CTRL_PAD][CTRLR_NUNCHUK], "btnmap_pad_nunchuk");
			loadXMLController(btnmap[CTRL_SCOPE][CTRLR_GCPAD], "btnmap_scope_gcpad");
			loadXMLController(btnmap[CTRL_SCOPE][CTRLR_WIIMOTE], "btnmap_scope_wiimote");
			loadXMLController(btnmap[CTRL_MOUSE][CTRLR_GCPAD], "btnmap_mouse_gcpad");
			loadXMLController(btnmap[CTRL_MOUSE][CTRLR_WIIMOTE], "btnmap_mouse_wiimote");
			loadXMLController(btnmap[CTRL_JUST][CTRLR_GCPAD], "btnmap_just_gcpad");
			loadXMLController(btnmap[CTRL_JUST][CTRLR_WIIMOTE], "btnmap_just_wiimote");

		}
		mxmlDelete(xml);
	}
	return result;
}

/****************************************************************************
 * FixInvalidSettings
 *
 * Attempts to correct at least some invalid settings - the ones that
 * might cause crashes
 ***************************************************************************/
void FixInvalidSettings()
{
	if(GCSettings.LoadMethod > 6)
		GCSettings.LoadMethod = DEVICE_AUTO;
	if(GCSettings.SaveMethod > 6)
		GCSettings.SaveMethod = DEVICE_AUTO;	
	if(!(GCSettings.zoomHor > 0.5 && GCSettings.zoomHor < 1.5))
		GCSettings.zoomHor = 1.0;
	if(!(GCSettings.zoomVert > 0.5 && GCSettings.zoomVert < 1.5))
		GCSettings.zoomVert = 1.0;
	if(!(GCSettings.xshift > -50 && GCSettings.xshift < 50))
		GCSettings.xshift = 0;
	if(!(GCSettings.yshift > -50 && GCSettings.yshift < 50))
		GCSettings.yshift = 0;
	if(!(GCSettings.MusicVolume >= 0 && GCSettings.MusicVolume <= 100))
		GCSettings.MusicVolume = 80;
	if(!(GCSettings.SFXVolume >= 0 && GCSettings.SFXVolume <= 100))
		GCSettings.SFXVolume = 20;
	if(GCSettings.language < 0 || GCSettings.language >= LANG_LENGTH)
		GCSettings.language = LANG_ENGLISH;
	if(GCSettings.Controller > CTRL_PAD4 || GCSettings.Controller < CTRL_SCOPE)
		GCSettings.Controller = CTRL_PAD2;
	if(!(GCSettings.videomode >= 0 && GCSettings.videomode < 6))
		GCSettings.videomode = 0;
	if(!(GCSettings.render >= 0 && GCSettings.render < 2))
		GCSettings.render = 0;
}

/****************************************************************************
 * DefaultSettings
 *
 * Sets all the defaults!
 ***************************************************************************/
void
DefaultSettings ()
{
	memset (&GCSettings, 0, sizeof (GCSettings));

	ResetControls(); // Controller button mappings

	GCSettings.Controller = CTRL_PAD2; // SNES Controllers, Super Scope, Konami Justifier, SNES Mouse
	GCSettings.FastForward = 1; // Enabled by default
	GCSettings.FastForwardButton = 0; // Right analog stick

	GCSettings.videomode = 0; // Automatic video mode detection
	GCSettings.render = 0; // Default rendering mode
	GCSettings.bilinear = 0; // Disabled by default
	GCSettings.VideoFilter = FILTER_NONE; // No video filter
	GCSettings.crosshair = 1; // Enabled by default

	GCSettings.aspect = 0;

#ifdef HW_RVL
	if (CONF_GetAspectRatio() == CONF_ASPECT_16_9)
		GCSettings.aspect = 1;
#endif

	GCSettings.zoomHor = 1.0; // Horizontal zoom level
	GCSettings.zoomVert = 1.0; // Vertical zoom level
	GCSettings.xshift = 0; // Horizontal video shift
	GCSettings.yshift = 0; // Vertical video shift

	GCSettings.Satellaview = 1; // Enabled by default

	GCSettings.WiimoteOrientation = 0;
	GCSettings.ExitAction = 0;
	GCSettings.AutoloadGame = 0;
	GCSettings.MusicVolume = 80;
	GCSettings.SFXVolume = 20;
	GCSettings.PreviewImage = 0;
	GCSettings.HideSRAMSaving = 0;

#ifdef HW_RVL
	GCSettings.language = CONF_GetLanguage();

	if(GCSettings.language == LANG_TRAD_CHINESE)
		GCSettings.language = LANG_SIMP_CHINESE;
#else
	GCSettings.language = LANG_ENGLISH;
#endif

	GCSettings.LoadMethod = DEVICE_AUTO; // Auto, SD, DVD, USB
	GCSettings.SaveMethod = DEVICE_AUTO; // Auto, SD, USB
	sprintf (GCSettings.LoadFolder, "%s/roms", APPFOLDER); // Path to game files
	sprintf (GCSettings.SaveFolder, "%s/saves", APPFOLDER); // Path to save files
	sprintf (GCSettings.CheatFolder, "%s/cheats", APPFOLDER); // Path to cheat files
	sprintf (GCSettings.ScreenshotsFolder, "%s/screenshots", APPFOLDER); // Path to screenshot files
	sprintf (GCSettings.CoverFolder, "%s/covers", APPFOLDER); // Path to cover files
	sprintf (GCSettings.ArtworkFolder, "%s/artwork", APPFOLDER); // Path to artwork files
	GCSettings.AutoLoad = 1; // Auto Load SRAM
	GCSettings.AutoSave = 1; // Auto Save SRAM

	/****************** SNES9x Settings ***********************/

	// Default ALL to false
	memset (&Settings, 0, sizeof (Settings));

	// General
	Settings.MouseMaster = false;
	Settings.SuperScopeMaster = false;
	Settings.JustifierMaster = false;
	Settings.MultiPlayer5Master = false;
	Settings.DontSaveOopsSnapshot = true;
	Settings.ApplyCheats = true;

	Settings.HDMATimingHack = 100;
	Settings.IsPatched = 0;

	// Sound
	Settings.SoundSync = true;
	Settings.SixteenBitSound = true;
	Settings.Stereo = true;
	Settings.ReverseStereo = true;
	Settings.SoundPlaybackRate = 48000;
	Settings.SoundInputRate = 31920;
	Settings.DynamicRateControl = true;

	GCSettings.MuteSound = 0; // Disabled by default
	GCSettings.Interpolation = 0;
	Settings.InterpolationMethod = DSP_INTERPOLATION_GAUSSIAN;

	// Graphics
	Settings.Transparency = true;
	Settings.TurboSkipFrames = 19;
	Settings.AutoDisplayMessages = false;
	Settings.InitialInfoStringTimeout = 0; // Display length of messages
	Settings.DisplayTime = false;

	GCSettings.HiResMode = 1; // Enabled by default
	GCSettings.FrameSkip = 1; // Enabled by default
	GCSettings.ShowFrameRate = 0; // Disabled by default

	// ROM timing
	Settings.FrameTimePAL = 20000;
	Settings.FrameTimeNTSC = 16667;

	// Hacks
	Settings.BlockInvalidVRAMAccessMaster = true;
	Settings.SeparateEchoBuffer = false;

	GCSettings.sfxOverclock = 0;
	/* Initialize SuperFX chip to normal speed by default */
	Settings.SuperFXSpeedPerLine = 5823405;
	Settings.SuperFXClockMultiplier = 100;

	GCSettings.cpuOverclock = 0;
	/* Initialize CPU to normal speed by default */
	Settings.OneClockCycle = 6;
	Settings.OneSlowClockCycle = 8;
	Settings.TwoClockCycles = 12;

	GCSettings.NoSpriteLimit = 0; // Disabled by default
}

/****************************************************************************
 * Save Preferences
 ***************************************************************************/
static char prefpath[MAXPATHLEN] = { 0 };

bool
SavePrefs (bool silent)
{
	char filepath[MAXPATHLEN];
	int datasize;
	int offset = 0;
	int device = 0;
	
	if(prefpath[0] != 0)
	{
		sprintf(filepath, "%s/%s", prefpath, PREF_FILE_NAME);
		FindDevice(filepath, &device);
	}
	else if(appPath[0] != 0)
	{
		sprintf(filepath, "%s/%s", appPath, PREF_FILE_NAME);
		strcpy(prefpath, appPath);
		FindDevice(filepath, &device);
	}
	else
	{
		device = autoSaveMethod(silent);
		
		if(device == 0)
			return false;
		
		sprintf(filepath, "%s%s", pathPrefix[device], APPFOLDER);
		DIR *dir = opendir(filepath);
		if (!dir)
		{
			if(mkdir(filepath, 0777) != 0)
				return false;
			sprintf(filepath, "%s%s/roms", pathPrefix[device], APPFOLDER);
			if(mkdir(filepath, 0777) != 0)
				return false;
			sprintf(filepath, "%s%s/saves", pathPrefix[device], APPFOLDER);
			if(mkdir(filepath, 0777) != 0)
				return false;
		}
		else
		{
			closedir(dir);
		}
		sprintf(filepath, "%s%s/%s", pathPrefix[device], APPFOLDER, PREF_FILE_NAME);
		sprintf(prefpath, "%s%s", pathPrefix[device], APPFOLDER);
	}
	
	if(device == 0)
		return false;

	if (!silent)
		ShowAction ("Saving preferences...");

	FixInvalidSettings();

	AllocSaveBuffer ();
	datasize = preparePrefsData ();

	offset = SaveFile(filepath, datasize, silent);

	FreeSaveBuffer ();

	CancelAction();

	if (offset > 0)
	{
		if (!silent)
			InfoPrompt("Preferences saved");
		return true;
	}
	return false;
}

/****************************************************************************
 * Load Preferences from specified filepath
 ***************************************************************************/
bool
LoadPrefsFromMethod (char * path)
{
	bool retval = false;
	int offset = 0;
	char filepath[MAXPATHLEN];
	sprintf(filepath, "%s/%s", path, PREF_FILE_NAME);

	AllocSaveBuffer ();

	offset = LoadFile(filepath, SILENT);

	if (offset > 0)
		retval = decodePrefsData ();

	FreeSaveBuffer ();
	
	if(retval)
	{
		strcpy(prefpath, path);

		if(appPath[0] == 0)
			strcpy(appPath, prefpath);
	}

	return retval;
}

/****************************************************************************
 * Load Preferences
 * Checks sources consecutively until we find a preference file
 ***************************************************************************/
static bool prefLoaded = false;

bool LoadPrefs()
{
	if(prefLoaded) // already attempted loading
		return true;

	bool prefFound = false;
	char filepath[5][MAXPATHLEN];
	int numDevices;
	bool sdMounted = false;
	bool usbMounted = false;

#ifdef HW_RVL
	numDevices = 5;
	sprintf(filepath[0], "%s", appPath);
	sprintf(filepath[1], "sd:/apps/%s", APPFOLDER);
	sprintf(filepath[2], "usb:/apps/%s", APPFOLDER);
	sprintf(filepath[3], "sd:/%s", APPFOLDER);
	sprintf(filepath[4], "usb:/%s", APPFOLDER);

	for(int i=0; i<numDevices; i++)
	{
		prefFound = LoadPrefsFromMethod(filepath[i]);
		
		if(prefFound)
			break;
	}
#else
	if(ChangeInterface(DEVICE_SD_SLOTA, SILENT)) {
		sprintf(filepath[0], "carda:/%s", APPFOLDER);
		prefFound = LoadPrefsFromMethod(filepath[0]);
	}
	else if(ChangeInterface(DEVICE_SD_SLOTB, SILENT)) {
		sprintf(filepath[0], "cardb:/%s", APPFOLDER);
		prefFound = LoadPrefsFromMethod(filepath[0]);
	}
	else if(ChangeInterface(DEVICE_SD_PORT2, SILENT)) {
		sprintf(filepath[0], "port2:/%s", APPFOLDER);
		prefFound = LoadPrefsFromMethod(filepath[0]);
	}
#endif

	prefLoaded = true; // attempted to load preferences

	if(prefFound) {
		FixInvalidSettings();
	}
	
	// rename snes9x to snes9xfx
	if(GCSettings.LoadMethod == DEVICE_SD)
	{
		sdMounted = ChangeInterface(DEVICE_SD, NOTSILENT);
		if(sdMounted && opendir("sd:/snes9x"))
			rename("sd:/snes9x", "sd:/snes9xfx");
	}
	else if(GCSettings.LoadMethod == DEVICE_USB)
	{
		usbMounted = ChangeInterface(DEVICE_USB, NOTSILENT);
		if(usbMounted && opendir("usb:/snes9x"))
			rename("usb:/snes9x", "usb:/snes9xfx");	
	}

	// update folder locations
	if(strcmp(GCSettings.LoadFolder, "snes9x/roms") == 0)
		sprintf(GCSettings.LoadFolder, "snes9xfx/roms");
	
	if(strcmp(GCSettings.SaveFolder, "snes9x/saves") == 0)
		sprintf(GCSettings.SaveFolder, "snes9xfx/saves");
	
	if(strcmp(GCSettings.CheatFolder, "snes9x/cheats") == 0)
		sprintf(GCSettings.CheatFolder, "snes9xfx/cheats");
		
	if(strcmp(GCSettings.ScreenshotsFolder, "snes9x/screenshots") == 0)
		sprintf(GCSettings.ScreenshotsFolder, "snes9xfx/screenshots");

	if(strcmp(GCSettings.CoverFolder, "snes9x/covers") == 0)
		sprintf(GCSettings.CoverFolder, "snes9xfx/covers");
	
	if(strcmp(GCSettings.ArtworkFolder, "snes9x/artwork") == 0)
		sprintf(GCSettings.ArtworkFolder, "snes9xfx/artwork");
	
	// attempt to create directories if they don't exist
	if((GCSettings.LoadMethod == DEVICE_SD && sdMounted) || (GCSettings.LoadMethod == DEVICE_USB && usbMounted) ) {
		char dirPath[MAXPATHLEN];
		sprintf(dirPath, "%s%s", pathPrefix[GCSettings.LoadMethod], GCSettings.ScreenshotsFolder);
		CreateDirectory(dirPath);
		sprintf(dirPath, "%s%s", pathPrefix[GCSettings.LoadMethod], GCSettings.CoverFolder);
		CreateDirectory(dirPath);
		sprintf(dirPath, "%s%s", pathPrefix[GCSettings.LoadMethod], GCSettings.ArtworkFolder);
		CreateDirectory(dirPath);
		sprintf(dirPath, "%s%s", pathPrefix[GCSettings.LoadMethod], GCSettings.CheatFolder);
		CreateDirectory(dirPath);
	}

	if(GCSettings.videomode > 0) {
		ResetVideo_Menu();
	}

	ChangeLanguage();

#ifdef HW_RVL
	bg_music = (u8 * )bg_music_ogg;
	bg_music_size = bg_music_ogg_size;
	LoadBgMusic();
#endif
	return prefFound;
}
