#include "c_cvars.h"
#include "config.hpp"
#include "wl_def.h"
#include "id_sd.h"
#include "id_in.h"
#include "id_us.h"

boolean r_depthfog = false;
boolean vid_fullscreen = true;

/*
====================
=
= ReadConfig
=
====================
*/

void ReadConfig(void)
{
    SDMode  sd;
    SMMode  sm;
    SDSMode sds;

	config->CreateSetting("MouseEnabled", 1);
	config->CreateSetting("JoystickEnabled", 0);
	config->CreateSetting("ViewSize", 19);
	config->CreateSetting("MouseAdjustment", 5);
	config->CreateSetting("SoundDevice", sdm_AdLib);
	config->CreateSetting("MusicDevice", smm_AdLib);
	config->CreateSetting("DigitalSoundDevice", sds_SoundBlaster);
	config->CreateSetting("AlwaysRun", 0);
	config->CreateSetting("MouseYAxisDisabled", 0);
	config->CreateSetting("SoundVolume", MAX_VOLUME);
	config->CreateSetting("MusicVolume", MAX_VOLUME);
	config->CreateSetting("DigitizedVolume", MAX_VOLUME);
	config->CreateSetting("R_DepthFog", false);
	config->CreateSetting("Vid_FullScreen", true);

#ifdef _arch_dreamcast
    DC_LoadFromVMU("ecwolf.cfg");
#endif

	char joySettingName[50] = {0};
	char keySettingName[50] = {0};
	char mseSettingName[50] = {0};
	mouseenabled = config->GetSetting("MouseEnabled")->GetInteger() != 0;
	joystickenabled = config->GetSetting("JoystickEnabled")->GetInteger() != 0;
	for(unsigned int i = 0;controlScheme[i].button != bt_nobutton;i++)
	{
		sprintf(joySettingName, "Joystick_%s", controlScheme[i].name);
		sprintf(keySettingName, "Keybaord_%s", controlScheme[i].name);
		sprintf(mseSettingName, "Mouse_%s", controlScheme[i].name);
		for(unsigned int j = 0;j < 50;j++)
		{
			if(joySettingName[j] == ' ')
				joySettingName[j] = '_';
			if(keySettingName[j] == ' ')
				keySettingName[j] = '_';
			if(mseSettingName[j] == ' ')
				mseSettingName[j] = '_';
		}
		config->CreateSetting(joySettingName, controlScheme[i].joystick);
		config->CreateSetting(keySettingName, controlScheme[i].keyboard);
		config->CreateSetting(mseSettingName, controlScheme[i].mouse);
		controlScheme[i].joystick = config->GetSetting(joySettingName)->GetInteger();
		controlScheme[i].keyboard = config->GetSetting(keySettingName)->GetInteger();
		controlScheme[i].mouse = config->GetSetting(mseSettingName)->GetInteger();
	}
	viewsize = config->GetSetting("ViewSize")->GetInteger();
	mouseadjustment = config->GetSetting("MouseAdjustment")->GetInteger();
	mouseyaxisdisabled = config->GetSetting("MouseYAxisDisabled")->GetInteger() != 0;
	alwaysrun = config->GetSetting("AlwaysRun")->GetInteger() != 0;
	sd = static_cast<SDMode> (config->GetSetting("SoundDevice")->GetInteger());
	sm = static_cast<SMMode> (config->GetSetting("MusicDevice")->GetInteger());
	sds = static_cast<SDSMode> (config->GetSetting("DigitalSoundDevice")->GetInteger());
	AdlibVolume = config->GetSetting("SoundVolume")->GetInteger();
	MusicVolume = config->GetSetting("MusicVolume")->GetInteger();
	SoundVolume = config->GetSetting("DigitizedVolume")->GetInteger();
	r_depthfog = config->GetSetting("R_DepthFog")->GetInteger() != 0;
	vid_fullscreen = config->GetSetting("Vid_FullScreen")->GetInteger() != 0;

	char hsName[50];
	char hsScore[50];
	char hsCompleted[50];
	char hsEpisode[50];
	for(unsigned int i = 0;i < MaxScores;i++)
	{
		sprintf(hsName, "HighScore%u_Name", i);
		sprintf(hsScore, "HighScore%u_Score", i);
		sprintf(hsCompleted, "HighScore%u_Completed", i);
		sprintf(hsEpisode, "HighScore%u_Episode", i);

		config->CreateSetting(hsName, Scores[i].name);
		config->CreateSetting(hsScore, Scores[i].score);
		config->CreateSetting(hsCompleted, Scores[i].completed);
		config->CreateSetting(hsEpisode, Scores[i].episode);

		strcpy(Scores[i].name, config->GetSetting(hsName)->GetString().c_str());
		Scores[i].score = config->GetSetting(hsScore)->GetInteger();
		Scores[i].completed = config->GetSetting(hsCompleted)->GetInteger();
		Scores[i].episode = config->GetSetting(hsEpisode)->GetInteger();
	}

	if ((sd == sdm_AdLib || sm == smm_AdLib) && !AdLibPresent
			&& !SoundBlasterPresent)
	{
		sd = sdm_PC;
		sm = smm_Off;
	}

	if ((sds == sds_SoundBlaster && !SoundBlasterPresent))
		sds = sds_Off;

	// make sure values are correct

	if(mouseenabled) mouseenabled=true;
	if(joystickenabled) joystickenabled=true;

	if (!MousePresent)
		mouseenabled = false;
	if (!IN_JoyPresent())
		joystickenabled = false;

	if(mouseadjustment<0) mouseadjustment=0;
	else if(mouseadjustment>20) mouseadjustment=20;

	if(viewsize<4) viewsize=4;
	else if(viewsize>21) viewsize=21;

    SD_SetMusicMode (sm);
    SD_SetSoundMode (sd);
    SD_SetDigiDevice (sds);
}

/*
====================
=
= WriteConfig
=
====================
*/

void WriteConfig(void)
{
#ifdef _arch_dreamcast
    fs_unlink("ecwolf.cfg");
#endif

	char joySettingName[50];
	char keySettingName[50];
	char mseSettingName[50];
	config->GetSetting("MouseEnabled")->SetValue(mouseenabled);
	config->GetSetting("JoystickEnabled")->SetValue(joystickenabled);
	for(unsigned int i = 0;controlScheme[i].button != bt_nobutton;i++)
	{
		sprintf(joySettingName, "Joystick_%s", controlScheme[i].name);
		sprintf(keySettingName, "Keybaord_%s", controlScheme[i].name);
		sprintf(mseSettingName, "Mouse_%s", controlScheme[i].name);
		for(unsigned int j = 0;j < 50;j++)
		{
			if(joySettingName[j] == ' ')
				joySettingName[j] = '_';
			if(keySettingName[j] == ' ')
				keySettingName[j] = '_';
			if(mseSettingName[j] == ' ')
				mseSettingName[j] = '_';
		}
		config->GetSetting(joySettingName)->SetValue(controlScheme[i].joystick);
		config->GetSetting(keySettingName)->SetValue(controlScheme[i].keyboard);
		config->GetSetting(mseSettingName)->SetValue(controlScheme[i].mouse);
	}
	config->GetSetting("ViewSize")->SetValue(viewsize);
	config->GetSetting("MouseAdjustment")->SetValue(mouseadjustment);
	config->GetSetting("MouseYAxisDisabled")->SetValue(mouseyaxisdisabled);
	config->GetSetting("AlwaysRun")->SetValue(alwaysrun);
	config->GetSetting("SoundDevice")->SetValue(SoundMode);
	config->GetSetting("MusicDevice")->SetValue(MusicMode);
	config->GetSetting("DigitalSoundDevice")->SetValue(DigiMode);
	config->GetSetting("SoundVolume")->SetValue(AdlibVolume);
	config->GetSetting("MusicVolume")->SetValue(MusicVolume);
	config->GetSetting("DigitizedVolume")->SetValue(SoundVolume);
	config->GetSetting("R_DepthFog")->SetValue(r_depthfog);
	config->GetSetting("Vid_FullScreen")->SetValue(vid_fullscreen);

	char hsName[50];
	char hsScore[50];
	char hsCompleted[50];
	char hsEpisode[50];
	for(unsigned int i = 0;i < MaxScores;i++)
	{
		sprintf(hsName, "HighScore%u_Name", i);
		sprintf(hsScore, "HighScore%u_Score", i);
		sprintf(hsCompleted, "HighScore%u_Completed", i);
		sprintf(hsEpisode, "HighScore%u_Episode", i);

		config->GetSetting(hsName)->SetValue(Scores[i].name);
		config->GetSetting(hsScore)->SetValue(Scores[i].score);
		config->GetSetting(hsCompleted)->SetValue(Scores[i].completed);
		config->GetSetting(hsEpisode)->SetValue(Scores[i].episode);
	}

#ifdef _arch_dreamcast
    DC_SaveToVMU("ecwolf.cfg", 1);
#endif
}
