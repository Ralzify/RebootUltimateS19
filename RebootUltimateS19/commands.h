#pragma once

#include "reboot.h"
#include "FortPlayerControllerAthena.h"
#include "KismetSystemLibrary.h"
#include "AthenaBarrierObjective.h"
#include "FortAthenaMutator_Barrier.h"
#include "FortWeaponMeleeItemDefinition.h"
#include "builder.h"
#include "FortLootPackage.h"
#include "bots.h"
#include "FortAthenaMutator_Bots.h"
#include "ai.h"
#include "FortPawn.h"
#include "moderation.h"
#include "gui.h"

bool IsOperator(APlayerState* PlayerState, AFortPlayerController* PlayerController)
{
	auto& IP = PlayerState->GetSavedNetworkAddress();
	auto IPStr = IP.ToString();

	// std::cout << "IPStr: " << IPStr << '\n';

	if (IPStr == "127.0.0.1" || IPStr == "26.93.63.127" || IsOp(PlayerController))
	{
		return true;
	}

	return false;
}

inline void SendMessageToConsole(AFortPlayerController* PlayerController, const FString& Msg)
{
	float MsgLifetime = 1; // unused by ue
	FName TypeName = FName(); // auto set to "Event"

	static auto ClientMessageFn = FindObject<UFunction>(L"/Script/Engine.PlayerController.ClientMessage");
	struct
	{
		FString                                     S;                                                        // (Parm, ZeroConstructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		FName                                       Type;                                                     // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		float                                              MsgLifeTime;                                              // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	} APlayerController_ClientMessage_Params{Msg, TypeName, MsgLifetime};

	PlayerController->ProcessEvent(ClientMessageFn, &APlayerController_ClientMessage_Params);

	// auto brah = Msg.ToString();
	// LOG_INFO(LogDev, "{}", brah);
}

void ServerCheatHook(AFortPlayerControllerAthena* PlayerController, FString Msg)
{
	if (!Msg.Data.Data || Msg.Data.Num() <= 0)
		return;

	auto PlayerState = Cast<AFortPlayerStateAthena>(PlayerController->GetPlayerState());

	// std::cout << "aa!\n";

	if (!PlayerState || !IsOperator(PlayerState, PlayerController))
		return;

	std::vector<std::string> Arguments;
	auto OldMsg = Msg.ToString();

	auto ReceivingController = PlayerController; // for now
	auto ReceivingPlayerState = PlayerState; // for now

	auto firstBackslash = OldMsg.find_first_of("\\");
	auto lastBackslash = OldMsg.find_last_of("\\");

	static auto World_NetDriverOffset = GetWorld()->GetOffset("NetDriver");
	auto WorldNetDriver = GetWorld()->Get<UNetDriver*>(World_NetDriverOffset);
	auto& ClientConnections = WorldNetDriver->GetClientConnections();

	if (firstBackslash != std::string::npos && lastBackslash != std::string::npos)
	{
		if (firstBackslash != lastBackslash)
		{
			std::string player = OldMsg;

			player = player.substr(firstBackslash + 1, lastBackslash - firstBackslash - 1);

			for (int i = 0; i < ClientConnections.Num(); ++i)
			{
				static auto PlayerControllerOffset = ClientConnections.at(i)->GetOffset("PlayerController");
				auto CurrentPlayerController = Cast<AFortPlayerControllerAthena>(ClientConnections.at(i)->Get(PlayerControllerOffset));

				if (!CurrentPlayerController)
					continue;

				auto CurrentPlayerState = Cast<AFortPlayerStateAthena>(CurrentPlayerController->GetPlayerState());

				if (!CurrentPlayerState)
					continue;

				FString PlayerName = CurrentPlayerState->GetPlayerName();

				if (PlayerName.ToString() == player) // hopefully we arent on adifferent thread
				{
					ReceivingController = CurrentPlayerController;
					ReceivingPlayerState = CurrentPlayerState;
					PlayerName.Free();
					break;
				}

				PlayerName.Free();
			}
		}
		else
		{
			// SendMessageToConsole(PlayerController, L"Warning: You have a backslash but no ending backslash, was this by mistake? Executing on you.");
		}
	}

	if (!ReceivingController || !ReceivingPlayerState)
	{
		SendMessageToConsole(PlayerController, L"Unable to find player!");
		return;
	}

	{
		auto Message = Msg.ToString();

		size_t start = Message.find('\\');

		while (start != std::string::npos) // remove the playername
		{
			size_t end = Message.find('\\', start + 1);

			if (end == std::string::npos)
				break;

			Message.replace(start, end - start + 2, "");
			start = Message.find('\\');
		}

		int zz = 0;

		// std::cout << "Message Before: " << Message << '\n';

		while (Message.find(" ") != std::string::npos)
		{
			auto arg = Message.substr(0, Message.find(' '));
			Arguments.push_back(arg);
			// std::cout << std::format("[{}] {}\n", zz, arg);
			Message.erase(0, Message.find(' ') + 1);
			zz++;
		}

		// if (zz == 0)
		{
			Arguments.push_back(Message);
			// std::cout << std::format("[{}] {}\n", zz, Message);
			zz++;
		}

		// std::cout << "Message After: " << Message << '\n';
	}

	auto NumArgs = Arguments.size() == 0 ? 0 : Arguments.size() - 1;

	// std::cout << "NumArgs: " << NumArgs << '\n';

	// return;

	bool bSendHelpMessage = false;

	if (Arguments.size() >= 1)
	{
		auto& Command = Arguments[0];
		std::transform(Command.begin(), Command.end(), Command.begin(), ::tolower);

		if (Command == "printsimulatelootdrops")
		{
			if (NumArgs < 1)
			{
				SendMessageToConsole(PlayerController, L"Please provide a LootTierGroup!");
				return;
			}

			auto& lootTierGroup = Arguments[1];

			auto LootDrops = PickLootDrops(UKismetStringLibrary::Conv_StringToName(std::wstring(lootTierGroup.begin(), lootTierGroup.end()).c_str()), -1, true);

			for (int i = 0; i < LootDrops.size(); ++i)
			{
				
			}

			SendMessageToConsole(PlayerController, L"Printed!");
		}
		/* else if (Command == "debugattributes")
		{
			auto AbilitySystemComponent = ReceivingPlayerState->GetAbilitySystemComponent();

			if (!AbilitySystemComponent)
			{
				SendMessageToConsole(PlayerController, L"No AbilitySystemComponent!");
				return;
			}

			SendMessageToConsole(PlayerController, (L"AbilitySystemComponent->GetSpawnedAttributes().Num(): " + std::to_wstring(AbilitySystemComponent->GetSpawnedAttributes().Num())).c_str());

			for (int i = 0; i < AbilitySystemComponent->GetSpawnedAttributes().Num(); ++i)
			{
				auto CurrentAttributePathName = AbilitySystemComponent->GetSpawnedAttributes().at(i)->GetPathName();
				SendMessageToConsole(PlayerController, (L"SpawnedAttribute Name: " + std::wstring(CurrentAttributePathName.begin(), CurrentAttributePathName.end())).c_str());
			}
		}
		else if (Command == "debugcurrentitem")
		{
			auto Pawn = ReceivingController->GetMyFortPawn();

			if (!Pawn)
			{
				SendMessageToConsole(PlayerController, L"No pawn!");
				return;
			}

			auto CurrentWeapon = Pawn->GetCurrentWeapon();

			if (!CurrentWeapon)
			{
				SendMessageToConsole(PlayerController, L"No CurrentWeapon!");
				return;
			}

			auto WorldInventory = ReceivingController->GetWorldInventory();

			if (!CurrentWeapon)
			{
				SendMessageToConsole(PlayerController, L"No WorldInventory!");
				return;
			}

			auto ItemInstance = WorldInventory->FindItemInstance(CurrentWeapon->GetItemEntryGuid());
			auto ReplicatedEntry = WorldInventory->FindReplicatedEntry(CurrentWeapon->GetItemEntryGuid());

			if (!ItemInstance)
			{
				SendMessageToConsole(PlayerController, L"Failed to find ItemInstance!");
				return;
			}

			if (!ReplicatedEntry)
			{
				SendMessageToConsole(PlayerController, L"Failed to find ReplicatedEntry!");
				return;
			}

			SendMessageToConsole(PlayerController, (L"ReplicatedEntry->GetGenericAttributeValues().Num(): " + std::to_wstring(ReplicatedEntry->GetGenericAttributeValues().Num())).c_str());
			SendMessageToConsole(PlayerController, (L"ReplicatedEntry->GetStateValues().Num(): " + std::to_wstring(ReplicatedEntry->GetStateValues().Num())).c_str());

			for (int i = 0; i < ReplicatedEntry->GetStateValues().Num(); ++i)
			{
				SendMessageToConsole(PlayerController, (L"[{}] StateValue Type: " 
					+ std::to_wstring((int)ReplicatedEntry->GetStateValues().at(i, FFortItemEntryStateValue::GetStructSize()).GetStateType())).c_str()
				);
			}
		} */
		else if (Command == "grant" || Command == "giveitem" || Command == "give")
		{
			if (NumArgs < 1)
			{
				SendMessageToConsole(PlayerController, L"Please provide an item!");
				return;
			}

			auto WorldInventory = ReceivingController->GetWorldInventory();

			if (!WorldInventory)
			{
				SendMessageToConsole(PlayerController, L"No world inventory!");
				return;
			}

			auto& weaponName = Arguments[1];
			int count = 1;

			try
			{
				if (NumArgs >= 2)
					count = std::stoi(Arguments[2]);
			}
			catch (...)
			{
			}

			if (weaponName == "ar_uc")
			{
				weaponName = "WID_Assault_Auto_Athena_UC_Ore_T03";
			}
			else if (weaponName == "ar_r" || weaponName == "ar")
			{
				weaponName = "WID_Assault_Auto_Athena_R_Ore_T03";
			}
			else if (weaponName == "ar_vr" || weaponName == "scar_vr")
			{
				weaponName = "WID_Assault_AutoHigh_Athena_VR_Ore_T03";
			}
			else if (weaponName == "ar_sr" || weaponName == "scar_sr" || weaponName == "scar")
			{
				weaponName = "WID_Assault_AutoHigh_Athena_SR_Ore_T03";
			}
			else if (weaponName == "tacar_r")
			{
				weaponName = "WID_Assault_PistolCaliber_AR_Athena_R_Ore_T03";
			}
			else if (weaponName == "tacar_vr")
			{
				weaponName = "WID_Assault_PistolCaliber_AR_Athena_VR_Ore_T03";
			}
			else if (weaponName == "tacar_sr" || weaponName == "tacar")
			{
				weaponName = "WID_Assault_PistolCaliber_AR_Athena_SR_Ore_T03";
			}
			else if (weaponName == "ar_ur" || weaponName == "scar_ur" || weaponName == "skyesar" || weaponName == "skyear")
			{
				weaponName = "WID_Boss_Adventure_AR";
			}
			else if (weaponName == "minigun_vr")
			{
				weaponName = "WID_Assault_LMG_Athena_VR_Ore_T03";
			}
			else if (weaponName == "minigun_sr" || weaponName == "minigun")
			{
				weaponName = "WID_Assault_LMG_Athena_SR_Ore_T03";
			}
			else if (weaponName == "pump_uc")
			{
				weaponName = "WID_Shotgun_Standard_Athena_C_Ore_T03";
			}
			else if (weaponName == "pump_r")
			{
				weaponName = "WID_Shotgun_Standard_Athena_UC_Ore_T03";
			}
			else if (weaponName == "pump_vr")
			{
				weaponName = "WID_Shotgun_Standard_Athena_VR_Ore_T03";
			}
			else if (weaponName == "pump_sr" || weaponName == "pump")
			{
				weaponName = "WID_Shotgun_Standard_Athena_SR_Ore_T03";
			}
			else if (weaponName == "tac_uc")
			{
				weaponName = "WID_Shotgun_SemiAuto_Athena_R_Ore_T03";
			}
			else if (weaponName == "tac_r")
			{
				weaponName = "WID_Shotgun_SemiAuto_Athena_VR_Ore_T03";
			}
			else if (weaponName == "tac_vr")
			{
				weaponName = "WID_Shotgun_HighSemiAuto_Athena_VR_Ore_T03";
			}
			else if (weaponName == "tac_sr" || weaponName == "tac")
			{
				weaponName = "WID_Shotgun_HighSemiAuto_Athena_SR_Ore_T03";
			}
			else if (weaponName == "flint_c")
			{
				weaponName = "WID_Pistol_Flintlock_Athena_C";
			}
			else if (weaponName == "flint_uc" || weaponName == "flint")
			{
				weaponName = "WID_Pistol_Flintlock_Athena_UC";
			}
			else if (weaponName == "flight" || weaponName == "flightknock" || weaponName == "yeetpistol")
			{
				weaponName = "Builder_WID_YEETknock_UR";
			}
			else if (weaponName == "deagle_vr")
			{
				weaponName = "WID_Pistol_HandCannon_Athena_VR_Ore_T03";
			}
			else if (weaponName == "deagle_sr" || weaponName == "deagle")
			{
				weaponName = "WID_Pistol_HandCannon_Athena_SR_Ore_T03";
			}
			else if (weaponName == "heavy_r")
			{
				weaponName = "WID_Sniper_Heavy_Athena_R_Ore_T03";
			}
			else if (weaponName == "heavy_vr")
			{
				weaponName = "WID_Sniper_Heavy_Athena_VR_Ore_T03";
			}
			else if (weaponName == "heavy_sr" || weaponName == "heavy")
			{
				weaponName = "WID_Sniper_Heavy_Athena_SR_Ore_T03";
			}
			else if (weaponName == "hunting_uc")
			{
				weaponName = "WID_Sniper_NoScope_Athena_UC_Ore_T03";
			}
			else if (weaponName == "hunting_r")
			{
				weaponName = "WID_Sniper_NoScope_Athena_R_Ore_T03";
			}
			else if (weaponName == "hunting_vr")
			{
				weaponName = "WID_Sniper_NoScope_Athena_VR_Ore_T03";
			}
			else if (weaponName == "hunting_sr")
			{
				weaponName = "WID_Sniper_NoScope_Athena_SR_Ore_T03";
			}
			else if (weaponName == "bolt_uc")
			{
				weaponName = "WID_Sniper_BoltAction_Scope_Athena_UC_Ore_T03";
			}
			else if (weaponName == "bolt_r" || weaponName == "bolt")
			{
				weaponName = "WID_Sniper_BoltAction_Scope_Athena_R_Ore_T03";
			}
			else if (weaponName == "bolt_vr")
			{
				weaponName = "WID_Sniper_BoltAction_Scope_Athena_VR_Ore_T03";
			}
			else if (weaponName == "bolt_sr")
			{
				weaponName = "WID_Sniper_BoltAction_Scope_Athena_SR_Ore_T03";
			}
			else if (weaponName == "suppressed_vr")
			{
				weaponName = "WID_Sniper_Suppressed_Scope_Athena_VR_Ore_T03";
			}
			else if (weaponName == "suppressed_sr" || weaponName == "suppressed")
			{
				weaponName = "WID_Sniper_Suppressed_Scope_Athena_SR_Ore_T03";
			}
			else if (weaponName == "semi_uc")
			{
				weaponName = "WID_Sniper_Standard_Scope_Athena_VR_Ore_T03";
			}
			else if (weaponName == "semi_r" || weaponName == "semi")
			{
				weaponName = "WID_Sniper_Standard_Scope_Athena_SR_Ore_T03";
			}
			else if (weaponName == "stormscout_vr")
			{
				weaponName = "WID_Sniper_Weather_Athena_VR";
			}
			else if (weaponName == "stormscout_sr" || weaponName == "stormscout")
			{
				weaponName = "WID_Sniper_Weather_Athena_SR";
			}
			else if (weaponName == "lever_uc")
			{
				weaponName = "WID_Sniper_Cowboy_Athena_UC";
			}
			else if (weaponName == "lever_r")
			{
				weaponName = "WID_Sniper_Cowboy_Athena_R";
			}
			else if (weaponName == "lever_vr" || weaponName == "lever")
			{
				weaponName = "WID_Sniper_Cowboy_Athena_VR";
			}
			else if (weaponName == "lever_sr")
			{
				weaponName = "WID_Sniper_Cowboy_Athena_SR";
			}
			else if (weaponName == "hunterbolt_uc")
			{
				weaponName = "WID_Sniper_CoreSniper_Athena_UC";
			}
			else if (weaponName == "hunterbolt_r")
			{
				weaponName = "WID_Sniper_CoreSniper_Athena_R";
			}
			else if (weaponName == "hunterbolt_vr")
			{
				weaponName = "WID_Sniper_CoreSniper_Athena_VR";
			}
			else if (weaponName == "hunterbolt_sr" || weaponName == "hunterbolt")
			{
				weaponName = "WID_Sniper_CoreSniper_Athena_SR";
			}
			else if (weaponName == "rocket_r")
			{
				weaponName = "WID_Launcher_Rocket_Athena_R_Ore_T03";
			}
			else if (weaponName == "rocket_vr")
			{
				weaponName = "WID_Launcher_Rocket_Athena_VR_Ore_T03";
			}
			else if (weaponName == "rocket_sr" || weaponName == "rocket")
			{
				weaponName = "WID_Launcher_Rocket_Athena_SR_Ore_T03";
			}
			else if (weaponName == "pumpkin_uc")
			{
				weaponName = "WID_Launcher_Pumpkin_Athena_UC_Ore_T03";
			}
			else if (weaponName == "pumpkin_r")
			{
				weaponName = "WID_Launcher_Pumpkin_Athena_R_Ore_T03";
			}
			else if (weaponName == "pumpkin_vr")
			{
				weaponName = "WID_Launcher_Pumpkin_Athena_VR_Ore_T03";
			}
			else if (weaponName == "pumpkin_sr" || weaponName == "pumpkin")
			{
				weaponName = "WID_Launcher_Pumpkin_Athena_SR_Ore_T03";
			}
			else if (weaponName == "gl_r")
			{
				weaponName = "WID_Launcher_Grenade_Athena_R_Ore_T03";
			}
			else if (weaponName == "gl_vr")
			{
				weaponName = "WID_Launcher_Grenade_Athena_VR_Ore_T03";
			}
			else if (weaponName == "gl_sr" || weaponName == "gl")
			{
				weaponName = "WID_Launcher_Grenade_Athena_SR_Ore_T03";
			}
			else if (weaponName == "quad_vr")
			{
				weaponName = "WID_Launcher_Military_Athena_VR_Ore_T03";
			}
			else if (weaponName == "quad_sr" || weaponName == "quad" || weaponName == "quadlauncher")
			{
				weaponName = "WID_Launcher_Military_Athena_SR_Ore_T03";
			}
			else if (weaponName == "guidedmissile_vr" || weaponName == "guided_vr" || weaponName == "missile_vr")
			{
				weaponName = "WID_RC_Rocket_Athena_VR_T03";
			}
			else if (weaponName == "guidedmissile_sr" || weaponName == "guided_sr" || weaponName == "missile_sr")
			{
				weaponName = "WID_RC_Rocket_Athena_SR_T03";
			}
			else if (weaponName == "xenonbow" || weaponName == "xenon" || weaponName == "stwbow")
			{
				weaponName = "WID_Sniper_Neon_Bow_SR_Crystal_T04";
			}
			else if (weaponName == "kits" || weaponName == "kitslauncher")
			{
				weaponName = "WID_Launcher_Shockwave_Athena_UR_Ore_T03";
			}
			else if (weaponName == "rift")
			{
				weaponName = "Athena_Rift_Item";
			}
			else if (weaponName == "spiderman")
			{
				weaponName = "WID_WestSausage_Parallel";
			}
			else if (weaponName == "crashpad" || weaponName == "crashes" || weaponName == "crash" || weaponName == "crashpads")
			{
				weaponName = "WID_Athena_AppleSun";
			}
			else if (weaponName == "chillers" || weaponName == "chiller" || weaponName == "chillergrenade")
			{
				weaponName = "Athena_IceGrenade";
			}
			else if (weaponName == "can" || weaponName == "rustycan" || weaponName == "cans")
			{
				weaponName = "WID_Athena_Bucket_Old";
			}
			else if (weaponName == "mythicgoldfish" || weaponName == "mythicfish" || weaponName == "goldfish")
			{
				weaponName = "WID_Athena_Bucket_Nice";
			}
			else if (weaponName == "coal")
			{
				weaponName = "WID_Athena_Bucket_Coal";
			}
			else if (weaponName == "stink" || weaponName == "stinkbomb" || weaponName == "stinks")
			{
				weaponName = "Athena_GasGrenade";
			}
			else if (weaponName == "fishingrod")
			{
				weaponName = "WID_Athena_FloppingRabbit";
			}
			else if (weaponName == "shieldbubble")
			{
				weaponName = "Athena_SilverBlazer_V2";
			}
			else if (weaponName == "zaptrap")
			{
				weaponName = "Athena_ZippyTrout";
			}
			else if (weaponName == "shockwave" || weaponName == "shock" || weaponName == "shockwavegrenade" || weaponName == "shocks" || weaponName == "shockwaves")
			{
				weaponName = "Athena_ShockGrenade";
			}
			else if (weaponName == "impulse" || weaponName == "impulsegrenade")
			{
				weaponName = "Athena_KnockGrenade";
			}
			else if (weaponName == "portafortress")
			{
				weaponName = "Athena_SuperTowerGrenade_A";
			}
			else if (weaponName == "hopflop" || weaponName == "hopflopper")
			{
				weaponName = "WID_Athena_Flopper_HopFlopper";
			}
			else if (weaponName == "snowyflopper" || weaponName == "snowyfish")
			{
				weaponName = "WID_Athena_Flopper_Snowman";
			}
			else if (weaponName == "slurpfish")
			{
				weaponName = "WID_Athena_Flopper_Effective";
			}
			else if (weaponName == "pizza")
			{
				weaponName = "WID_Athena_PizzaParty";
			}
			else if (weaponName == "keg" || weaponName == "shieldkeg" || weaponName == "sprinkler" || Command == "shieldsprinkler")
			{
				weaponName = "WID_Athena_ShieldGenerator";
			}
			else if (weaponName == "thermalflopper" || weaponName == "thermalfish")
			{
				weaponName = "WID_Athena_Flopper_Thermal";
			}
			else if (weaponName == "zeropoint" || weaponName == "zeropointfish")
			{
				weaponName = "WID_Athena_Flopper_Zero";
			}
			else if (weaponName == "chugsplash" || weaponName == "chug" || weaponName == "chugs")
			{
				weaponName = "Athena_ChillBronco";
			}
			else if (weaponName == "minis")
			{
				weaponName = "Athena_ShieldSmall";
			}
			else if (weaponName == "bandage" || weaponName == "bandages")
			{
				weaponName = "Athena_Bandage";
			}
			else if (weaponName == "portafort")
			{
				weaponName = "Athena_TowerGrenade";
			}
			else if (weaponName == "c4")
			{
				weaponName = "Athena_C4";
			}
			else if (weaponName == "stormflip")
			{
				weaponName = "Athena_DogSweater";
			}
			else if (weaponName == "firefly")
			{
				weaponName = "WID_Athena_Grenade_Molotov";
			}
			else if (weaponName == "tire" || weaponName == "tires" || weaponName == "tyre")
			{
				weaponName = "ID_ValetMod_Tires_OffRoad_Thrown";
			}
			else if (weaponName == "doomgauntlets" || weaponName == "doom")
			{
				weaponName = "WID_HighTower_Date_ChainLightning_CoreBR";
			}
			else if (weaponName == "dub")
			{
				weaponName = "WID_WaffleTruck_Dub";
			}
			else if (weaponName == "hoprockdualies" || weaponName == "dualies")
			{
				weaponName = "WID_WaffleTruck_HopRockDualies";
			}
			else if (weaponName == "bigchill")
			{
				weaponName = "WID_WaffleTruck_ChillerLauncher";
			}
			else if (weaponName == "firesniper")
			{
				weaponName = "WID_WaffleTruck_Sniper_DragonBreath";
			}
			else if (weaponName == "recon")
			{
				weaponName = "AGID_Athena_Scooter";
			}
			else if (weaponName == "harpoon")
			{
				weaponName = "WID_Athena_HappyGhost_Infinite";
			}
			else if (weaponName == "jules" || weaponName == "julesgrappler" || weaponName == "julesgrap")
			{
				weaponName = "WID_Boss_GrapplingHoot";
			}
			else if (weaponName == "skyesgrappler" || weaponName == "skyegrap" || weaponName == "skyesgrap")
			{
				weaponName = "WID_Boss_Adventure_GH";
			}
			else if (weaponName == "captainamerica" || weaponName == "shield" || weaponName == "ca")
			{
				weaponName = "AGID_AshtonPack_Chicago";
			}
			else if (weaponName == "thorshammer" || weaponName == "thor" || weaponName == "thors")
			{
				weaponName = "AGID_AshtonPack_Turbo";
			}
			else if (weaponName == "batman" || weaponName == "batgrap")
			{
				weaponName = "WID_Badger_Grape_VR";
			}
			else if (weaponName == "batarangs")
			{
				weaponName = "WID_Athena_BadgerBangsNew";
			}
			else if (weaponName == "stwpumpkin" || weaponName == "stwrocket")
			{
				weaponName = "WID_Launcher_Pumpkin_RPG_SR_Ore_T01";
			}
			else if (weaponName == "grabitron")
			{
				weaponName = "WID_GravyGoblinV2_Athena";
			}
			else if (weaponName == "bouncer" || weaponName == "bouncers")
			{
				weaponName = "TID_Context_BouncePad_Athena";
			}
			else if (weaponName == "launchpad" || weaponName == "launch" || weaponName == "pad" || weaponName == "launches")
			{
				weaponName = "TID_Floor_Player_Launch_Pad_Athena";
			}
			else if (weaponName == "grappler" || weaponName == "grap" || weaponName == "grapple")
			{
				if (Fortnite_Version < 7.10)
				{
					weaponName = "WID_Hook_Gun_VR_Ore_T03";
				}
				else
				{
					weaponName = "WID_Hook_Gun_Slide";
				}
			}
			else if (weaponName == "presents" || weaponName == "present")
			{
				if (Fortnite_Version < 15)
				{
					weaponName = "Athena_GiftBox";
				}
				else
				{
					weaponName = "Athena_HolidayGiftBox";
				}
			}
			else if (weaponName == "balloons" || weaponName == "balloon")
			{
				if (Fortnite_Version < 7)
				{
					weaponName = "Athena_Balloons";
				}
				else
				{
					weaponName = "Athena_Balloons_Consumable";
				}
			}
			else if (weaponName == "snowman" || weaponName == "snowmen")
			{
				if (Fortnite_Version < 11)
				{
					weaponName = "Athena_SneakySnowman";
				}
				else
				{
					weaponName = "AGID_SneakySnowmanV2";
				}
			}
			else if (weaponName == "ironman" || weaponName == "iron-man")
			{
				if (Fortnite_Version < 14.00 || Fortnite_Version > 14.60)
				{
					weaponName = "AGID_AshtonPack_Indigo";
				}
				else
				{
					weaponName = "WID_HighTower_Tomato_Repulsor_CoreBR";
				}
			}
			else if (weaponName == "hunting")
			{
				if (Fortnite_Version < 12.41)
				{
					weaponName = "WID_Sniper_NoScope_Athena_R_Ore_T03";
				}
				else
				{
					weaponName = "WID_Sniper_NoScope_Athena_SR_Ore_T03";
				}
			}

			auto WID = Cast<UFortWorldItemDefinition>(FindObject(weaponName, nullptr, ANY_PACKAGE));

			if (!WID)
			{
				SendMessageToConsole(PlayerController, L"Invalid WID! This usually means you either have the wrong name of an item, or the item doesn't exist on your version!");
				return;
			}

			bool bShouldUpdate = false;
			WorldInventory->AddItem(WID, &bShouldUpdate, count);

			if (bShouldUpdate)
				WorldInventory->Update();

			SendMessageToConsole(PlayerController, L"Granted item!");
		}
		else if (Command == "op")
		{
			if (ReceivingController == PlayerController)
			{
				SendMessageToConsole(PlayerController, L"You can't op yourself!");
				return;
			}

			if (IsOp(ReceivingController))
			{
				SendMessageToConsole(PlayerController, L"Player is already operator!");
				return;
			}

			Op(ReceivingController);
			SendMessageToConsole(PlayerController, L"Granted operator to player!");
		}
		else if (Command == "deop")
		{
			if (!IsOp(ReceivingController))
			{
				SendMessageToConsole(PlayerController, L"Player is not operator!");
				return;
			}

			Deop(ReceivingController);
			SendMessageToConsole(PlayerController, L"Removed operator from player!");
		}
		else if (Command == "setpickaxe" || Command == "pickaxe")
		{
			if (NumArgs < 1)
			{
				SendMessageToConsole(PlayerController, L"Please provide a pickaxe!");
				return;
			}

			if (Fortnite_Version < 3) // Idk why but emptyslot kicks the player because of the validate.
			{
				SendMessageToConsole(PlayerController, L"Not supported on this version!");
				return;
			}

			auto WorldInventory = ReceivingController->GetWorldInventory();

			if (!WorldInventory)
			{
				SendMessageToConsole(PlayerController, L"No world inventory!");
				return;
			}

			auto& pickaxeName = Arguments[1];
			static auto AthenaPickaxeItemDefinitionClass = FindObject<UClass>("/Script/FortniteGame.AthenaPickaxeItemDefinition");

			auto Pickaxe1 = FindObject(pickaxeName + "." + pickaxeName, nullptr, ANY_PACKAGE);

			UFortWeaponMeleeItemDefinition* NewPickaxeItemDefinition = nullptr;

			if (Pickaxe1)
			{
				if (Pickaxe1->IsA(AthenaPickaxeItemDefinitionClass))
				{
					static auto WeaponDefinitionOffset = Pickaxe1->GetOffset("WeaponDefinition");
					NewPickaxeItemDefinition = Pickaxe1->Get<UFortWeaponMeleeItemDefinition*>(WeaponDefinitionOffset);
				}
				else
				{
					NewPickaxeItemDefinition = Cast<UFortWeaponMeleeItemDefinition>(Pickaxe1);
				}
			}

			if (!NewPickaxeItemDefinition)
			{
				SendMessageToConsole(PlayerController, L"Invalid pickaxe item definition!");
				return;
			}

			auto PickaxeInstance = WorldInventory->GetPickaxeInstance();
			
			if (PickaxeInstance)
			{
				WorldInventory->RemoveItem(PickaxeInstance->GetItemEntry()->GetItemGuid(), nullptr, PickaxeInstance->GetItemEntry()->GetCount(), true);
			}

			WorldInventory->AddItem(NewPickaxeItemDefinition, nullptr, 1);
			WorldInventory->Update();

			SendMessageToConsole(PlayerController, L"Successfully set pickaxe!");
		}
		else if (Command == "load")
		{
			if (!Globals::bCreative)
			{
				SendMessageToConsole(PlayerController, L"It is not creative!");
				return;
			}

			static auto CreativePlotLinkedVolumeOffset = ReceivingController->GetOffset("CreativePlotLinkedVolume", false);
			auto Volume = CreativePlotLinkedVolumeOffset == -1 ? nullptr : ReceivingController->GetCreativePlotLinkedVolume();

			if (Arguments.size() <= 1)
			{
				SendMessageToConsole(PlayerController, L"Please provide a filename!\n");
				return;
			}

			std::string FileName = "islandSave";

			try { FileName = Arguments[1]; }
			catch (...) {}

			float X{ -1 }, Y{ -1 }, Z{ -1 };

			if (Arguments.size() >= 4)
			{
				try { X = std::stof(Arguments[2]); }
				catch (...) {}
				try { Y = std::stof(Arguments[3]); }
				catch (...) {}
				try { Z = std::stof(Arguments[4]); }
				catch (...) {}
			}
			else
			{
				if (!Volume)
				{
					SendMessageToConsole(PlayerController, L"They do not have an island!");
					return;
				}
			}

			if (X != -1 && Y != -1 && Z != -1) // omg what if they want to spawn it at -1 -1 -1!!!
				Builder::LoadSave(FileName, FVector(X, Y, Z), FRotator());
			else
				Builder::LoadSave(FileName, Volume);

			SendMessageToConsole(PlayerController, L"Loaded!");
		}
		else if (Command == "spawnpickup")
		{
			if (NumArgs < 1)
			{
				SendMessageToConsole(PlayerController, L"Please provide a WID!");
				return;
			}

			auto Pawn = ReceivingController->GetMyFortPawn();

			if (!Pawn)
			{
				SendMessageToConsole(PlayerController, L"No pawn!");
				return;
			}

			auto& weaponName = Arguments[1];
			int count = 1;
			int amount = 1;

			try
			{
				if (NumArgs >= 2)
					count = std::stoi(Arguments[2]);
				if (NumArgs >= 3)
					amount = std::stoi(Arguments[3]);
			}
			catch (...)
			{
			}

			constexpr int Max = 100;

			if (amount > Max)
			{
				SendMessageToConsole(PlayerController, (std::wstring(L"You went over the limit! Only spawning ") + std::to_wstring(Max) + L".").c_str());
				amount = Max;
			}

			// LOG_INFO(LogDev, "weaponName: {}", weaponName);

			auto WID = Cast<UFortWorldItemDefinition>(FindObject(weaponName, nullptr, ANY_PACKAGE));

			if (!WID)
			{
				SendMessageToConsole(PlayerController, L"Invalid WID!");
				return;
			}

			auto Location = Pawn->GetActorLocation();

			auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());

			PickupCreateData CreateData;
			CreateData.ItemEntry = FFortItemEntry::MakeItemEntry(WID, count, -1, MAX_DURABILITY, WID->GetFinalLevel(GameState->GetWorldLevel()));
			CreateData.SpawnLocation = Location;
			CreateData.bShouldFreeItemEntryWhenDeconstructed = true;

			for (int i = 0; i < amount; i++)
			{
				AFortPickup::SpawnPickup(CreateData);
			}
		}
		else if (Command == "listplayers")
		{
			std::string PlayerNames;

			for (int i = 0; i < ClientConnections.Num(); i++)
			{
				static auto PlayerControllerOffset = ClientConnections.at(i)->GetOffset("PlayerController");
				auto CurrentPlayerController = Cast<AFortPlayerControllerAthena>(ClientConnections.at(i)->Get(PlayerControllerOffset));

				if (!CurrentPlayerController)
					continue;

				auto CurrentPlayerState = Cast<AFortPlayerStateAthena>(CurrentPlayerController->GetPlayerState());

				if (!CurrentPlayerState->IsValidLowLevel())
					continue;

				PlayerNames += "\"" + CurrentPlayerState->GetPlayerName().ToString() + "\" ";
			}

			SendMessageToConsole(PlayerController, std::wstring(PlayerNames.begin(), PlayerNames.end()).c_str());
		}
		else if (Command == "launch" || Command == "fling")
		{
			if (Arguments.size() <= 3)
			{
				SendMessageToConsole(PlayerController, L"Please provide X, Y, and Z!\n");
				return;
			}

			float X{}, Y{}, Z{};

			try { X = std::stof(Arguments[1]); }
			catch (...) {}
			try { Y = std::stof(Arguments[2]); }
			catch (...) {}
			try { Z = std::stof(Arguments[3]); }
			catch (...) {}

			auto Pawn = ReceivingController->GetMyFortPawn();

			if (!Pawn)
			{
				SendMessageToConsole(PlayerController, L"No pawn to teleport!");
				return;
			}

			static auto LaunchCharacterFn = FindObject<UFunction>(L"/Script/Engine.Character.LaunchCharacter");

			struct 
			{
				FVector                                     LaunchVelocity;                                           // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				bool                                               bXYOverride;                                              // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				bool                                               bZOverride;                                               // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			} ACharacter_LaunchCharacter_Params{ FVector(X, Y, Z), false, false};
			Pawn->ProcessEvent(LaunchCharacterFn, &ACharacter_LaunchCharacter_Params);

			SendMessageToConsole(PlayerController, L"Launched character!");
		}
		else if (Command == "shield" || Command == "setshield")
		{
			auto Pawn = ReceivingController->GetMyFortPawn();

			if (!Pawn)
			{
				SendMessageToConsole(PlayerController, L"No pawn!");
				return;
			}

			float Shield = 0.f;

			if (NumArgs >= 1)
			{
				try { Shield = std::stof(Arguments[1]); }
				catch (...) {}
			}

			Pawn->SetShield(Shield);
			SendMessageToConsole(PlayerController, L"Set shield!\n");
		}
		else if (Command == "god")
		{
			auto Pawn = ReceivingController->GetMyFortPawn();

			if (!Pawn)
			{
				SendMessageToConsole(PlayerController, L"No pawn!");
				return;
			}

			Pawn->SetCanBeDamaged(!Pawn->CanBeDamaged());
			SendMessageToConsole(PlayerController, std::wstring(L"God set to " + std::to_wstring(!(bool)Pawn->CanBeDamaged())).c_str());
		}
		else if (Command == "applycid" || Command == "skin")
		{
			auto PlayerState = Cast<AFortPlayerState>(ReceivingController->GetPlayerState());

			if (!PlayerState) // ???
			{
				SendMessageToConsole(PlayerController, L"No playerstate!");
				return;
			}

			auto Pawn = Cast<AFortPlayerPawn>(ReceivingController->GetMyFortPawn());

			std::string CIDStr = Arguments[1];
			auto CIDDef = FindObject(CIDStr, nullptr, ANY_PACKAGE);
			// auto CIDDef = UObject::FindObject<UAthenaCharacterItemDefinition>(CIDStr);

			if (!CIDDef)
			{
				SendMessageToConsole(PlayerController, L"Invalid character item definition!");
				return;
			}

			LOG_INFO(LogDev, "Applying {}", CIDDef->GetFullName());
			
			if (!ApplyCID(Pawn, CIDDef))
			{
				SendMessageToConsole(PlayerController, L"Failed while applying skin! Please check the server log.");
				return;
			}

			SendMessageToConsole(PlayerController, L"Applied CID!");
		} 
		else if (Command == "spawn" || Command == "summon")
		{
			if (Arguments.size() <= 1)
			{
				SendMessageToConsole(PlayerController, L"Please provide a name or BP!\n");
				return;
			}

			auto& ActorName = Arguments[1];

			auto Pawn = ReceivingController->GetPawn();

			if (!Pawn)
			{
				SendMessageToConsole(PlayerController, L"No pawn to spawn BP at!");
				return;
			}

			int Count = 1;

			if (Arguments.size() >= 3)
			{
				try { Count = std::stod(Arguments[2]); }
				catch (...) {}
			}

			bool SpawnBots = ActorName == "bot" || ActorName == "bots";

			int Max = SpawnBots ? 99 : 100;

			if (Count > Max)
			{
				SendMessageToConsole(PlayerController, (std::wstring(L"You went over the limit! Only spawning ") + std::to_wstring(Max) + L".").c_str());
				Count = Max;
			}

			if (ActorName == "driftboard" || ActorName == "hoverboard")
				ActorName = "/Game/Athena/DrivableVehicles/JackalVehicle_Athena.JackalVehicle_Athena_C";
			else if (ActorName == "surfboard")
				ActorName = "/Game/Athena/DrivableVehicles/SurfboardVehicle_Athena.SurfboardVehicle_Athena_C";
			else if (ActorName == "quadcrasher" || ActorName == "quad")
				ActorName = "/Game/Athena/DrivableVehicles/AntelopeVehicle.AntelopeVehicle_C";
			else if (ActorName == "baller")
				ActorName = "/Game/Athena/DrivableVehicles/Octopus/OctopusVehicle.OctopusVehicle_C";
			else if (ActorName == "plane")
				ActorName = "/Game/Athena/DrivableVehicles/Biplane/BluePrints/FerretVehicle.FerretVehicle_C";
			else if (ActorName == "golfcart" || ActorName == "golf")
				ActorName = "/Game/Athena/DrivableVehicles/Golf_Cart/Golf_Cart_Base/Blueprints/GolfCartVehicleSK.GolfCartVehicleSK_C";
			else if (ActorName == "cannon")
				ActorName = "/Game/Athena/DrivableVehicles/PushCannon.PushCannon_C";
			else if (ActorName == "shoppingcart" || ActorName == "shopping")
				ActorName = "/Game/Athena/DrivableVehicles/ShoppingCartVehicleSK.ShoppingCartVehicleSK_C";
			else if (ActorName == "mech" || ActorName == "brute")
				ActorName = "/Game/Athena/DrivableVehicles/Mech/TestMechVehicle.TestMechVehicle_C";
			else if (ActorName == "bear" || ActorName == "truck")
				ActorName = "/Valet/BasicTruck/Valet_BasicTruck_Vehicle.Valet_BasicTruck_Vehicle_C";
			else if (ActorName == "prevelant" || ActorName == "car")
				ActorName = "/Valet/BasicCar/Valet_BasicCar_Vehicle.Valet_BasicCar_Vehicle_C";
			else if (ActorName == "whiplash" || ActorName == "sportscar")
				ActorName = "/Valet/SportsCar/Valet_SportsCar_Vehicle.Valet_SportsCar_Vehicle_C";
			else if (ActorName == "taxi")
				ActorName = "/Valet/TaxiCab/Valet_TaxiCab_Vehicle.Valet_TaxiCab_Vehicle_C";
			else if (ActorName == "mudflap")
				ActorName = "/Valet/BigRig/Valet_BigRig_Vehicle.Valet_BigRig_Vehicle_C";
			else if (ActorName == "stark")
				ActorName = "/Valet/SportsCar/Valet_SportsCar_Vehicle_HighTower.Valet_SportsCar_Vehicle_HighTower_C";
			else if (ActorName == "boat")
				ActorName = "/Game/Athena/DrivableVehicles/Meatball/Meatball_Large/MeatballVehicle_L.MeatballVehicle_L_C";
			else if (ActorName == "heli" || ActorName == "helicopter")
				ActorName = "/Hoagie/HoagieVehicle.HoagieVehicle_C";
			else if (ActorName == "ufo")
				ActorName = "/Nevada/Blueprints/Vehicle/Nevada_Vehicle_V2.Nevada_Vehicle_V2_C";
			else if (ActorName == "shark")
				ActorName = "/SpicySake/Pawns/NPC_Pawn_SpicySake_Parent.NPC_Pawn_SpicySake_Parent_C";
			else if (ActorName == "klombo")
				ActorName = "/ButterCake/Pawns/NPC_Pawn_ButterCake_Base.NPC_Pawn_ButterCake_Base_C";
			else if (ActorName == "umbrella")
				ActorName = "/Game/Athena/Apollo/Environments/BuildingActors/Papaya/Papaya_BouncyUmbrella_C.Papaya_BouncyUmbrella_C_C";
			else if (ActorName == "tire")
				ActorName = "/Game/Building/ActorBlueprints/Prop/Prop_TirePile_04.Prop_TirePile_04_C";
			else if (ActorName == "airvent")
				ActorName = "/Game/Athena/Environments/Blueprints/DUDEBRO/BGA_HVAC.BGA_HVAC_C";
			else if (ActorName == "geyser")
				ActorName = "/Game/Athena/Environments/Blueprints/DudeBro/BGA_DudeBro_Mini.BGA_DudeBro_Mini_C";
			else if (ActorName == "nobuildzone")
				ActorName = "/Game/Athena/Prototype/Blueprints/Galileo/BP_Galileo_NoBuildZone.BP_Galileo_NoBuildZone_C";
			else if (ActorName == "launch" || ActorName == "launchpad")
				ActorName = "/Game/Athena/Items/Traps/Launchpad/BluePrint/Trap_Floor_Player_Launch_Pad.Trap_Floor_Player_Launch_Pad_C";
			else if (ActorName == "rift")
				ActorName = "/Game/Athena/Items/ForagedItems/Rift/BGA_RiftPortal_Athena_Spawner.BGA_RiftPortal_Athena_Spawner_C";
			else if (ActorName == "supplydrop")
				if (Fortnite_Version >= 12.30 && Fortnite_Version <= 12.61)
					ActorName = "/Game/Athena/SupplyDrops/AthenaSupplyDrop_Donut.AthenaSupplyDrop_Donut_C";
				else if (Fortnite_Version == 5.10 || Fortnite_Version == 9.41 || Fortnite_Version == 14.20 || Fortnite_Version == 18.00)
					ActorName = "/Game/Athena/SupplyDrops/AthenaSupplyDrop_BDay.AthenaSupplyDrop_BDay_C";
				else if (Fortnite_Version == 1.11 || Fortnite_Version == 7.10 || Fortnite_Version == 7.20 || Fortnite_Version == 7.30 || Fortnite_Version == 11.31 || Fortnite_Version == 15.10 || Fortnite_Version == 19.01)
					ActorName = "/Game/Athena/SupplyDrops/AthenaSupplyDrop_Holiday.AthenaSupplyDrop_Holiday_C";
				else if (Fortnite_Version == 5.40 || Fortnite_Version == 5.41)
					ActorName = "/Game/Athena/SupplyDrops/Bling/AthenaSupplyDrop_Bling.AthenaSupplyDrop_Bling_C";
				else ActorName = "/Game/Athena/SupplyDrops/AthenaSupplyDrop.AthenaSupplyDrop_C";

			static auto BGAClass = FindObject<UClass>(L"/Script/Engine.BlueprintGeneratedClass");
			static auto ClassClass = FindObject<UClass>(L"/Script/CoreUObject.Class");
			auto ClassObj = ActorName.contains("/Script/") ? FindObject<UClass>(ActorName, ClassClass) : LoadObject<UClass>(ActorName, BGAClass);

			if (ClassObj || SpawnBots)
			{
				int AmountSpawned = 0;

				for (int i = 0; i < Count; i++)
				{
					auto Loc = Pawn->GetActorLocation();
					Loc.Z += 250;

					FTransform BotSpawnTransform;
					BotSpawnTransform.Translation = Loc;
					BotSpawnTransform.Scale3D = FVector(1, 1, 1);

					auto NewActor = SpawnBots ? Bots::SpawnBot(BotSpawnTransform) : GetWorld()->SpawnActor<AActor>(ClassObj, Loc, FQuat(), FVector(1, 1, 1));

					if (!NewActor)
					{
						SendMessageToConsole(PlayerController, (L"Failed to spawn a{}!", (SpawnBots ? L" bot" : L"n actor!")));
					}
					else
					{
						NewActor->ForceNetUpdate();
						AmountSpawned++;
					}
				}

				SendMessageToConsole(PlayerController, L"Summoned!");
			}
			else
			{
				SendMessageToConsole(PlayerController, L"Not a valid class!");
			}
		}
		else if (Command == "getlocation")
		{
			auto Pawn = ReceivingController->GetMyFortPawn();

			auto PawnLocation = Pawn->GetActorLocation();

			FString Loc = PawnLocation.ToString();

			SendMessageToConsole(PlayerController, Loc);

			Pawn->CopyToClipboard(Loc);
		}
		else if (Command == "dbno") // i kept misspelling it
		{
			auto Pawn = ReceivingController->GetMyFortPawn();

			if (!Pawn)
			{
				SendMessageToConsole(ReceivingController, L"No pawn!");
				return;
			}

			Pawn->SetDBNO(!Pawn->IsDBNO());
			SendMessageToConsole(PlayerController, std::wstring(L"DBNO set to " + std::to_wstring(!(bool)Pawn->IsDBNO())).c_str());
		}
		else if (Command == "spawnbottest")
		{
			// /Game/Athena/AI/MANG/BotData/

			if (NumArgs < 1)
			{
				SendMessageToConsole(PlayerController, L"Please provide a customization object!");
				return;
			}

			auto Pawn = ReceivingController->GetPawn();

			if (!Pawn)
			{
				SendMessageToConsole(PlayerController, L"No pawn to spawn bot at!");
				return;
			}

			auto CustomizationData = LoadObject<UFortAthenaAIBotCustomizationData>(Arguments[1], UFortAthenaAIBotCustomizationData::StaticClass());

			if (!CustomizationData)
			{
				SendMessageToConsole(PlayerController, L"Invalid CustomizationData!");
				return;
			}

			auto NewPawn = SpawnAIFromCustomizationData(Pawn->GetActorLocation(), CustomizationData);

			if (NewPawn)
			{
				SendMessageToConsole(PlayerController, L"Spawned!");
			}
			else
			{
				SendMessageToConsole(PlayerController, L"Failed to spawn!");
			}
		}
		else if (Command == "bot" || Command == "spawnbot")
		{
			auto Pawn = ReceivingController->GetPawn();

			if (!Pawn)
			{
				SendMessageToConsole(PlayerController, L"No pawn to spawn bot at!");
				return;
			}

			int Count = 1;

			if (Arguments.size() >= 2)
			{
				try { Count = std::stod(Arguments[1]); }
				catch (...) {}
			}

			constexpr int Max = 99;

			if (Count > Max)
			{
				SendMessageToConsole(PlayerController, (std::wstring(L"You went over the limit! Only spawning ") + std::to_wstring(Max) + L".").c_str());
				Count = Max;
			}

			int AmountSpawned = 0;

			for (int i = 0; i < Count; i++)
			{
				FActorSpawnParameters SpawnParameters{};
				// SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				auto Loc = Pawn->GetActorLocation();
				Loc.Z += 1000;

				FTransform Transform;
				Transform.Translation = Loc;
				Transform.Scale3D = FVector(1, 1, 1);

				auto NewActor = Bots::SpawnBot(Transform);

				if (!NewActor)
				{
					SendMessageToConsole(PlayerController, L"Failed to spawn an actor!");
				}
				else
				{
					AmountSpawned++;
				}
			}

			SendMessageToConsole(PlayerController, L"Summoned!");
		}
		else if (Command == "health" || Command == "sethealth")
		{
			auto Pawn = ReceivingController->GetMyFortPawn();

			if (!Pawn)
			{
				SendMessageToConsole(PlayerController, L"No pawn!");
				return;
			}

			float Health = 100.f;

			try { Health = std::stof(Arguments[1]); }
			catch (...) {}

			Pawn->SetHealth(Health);
			SendMessageToConsole(PlayerController, L"Set health!\n");
		}
		else if (Command == "pausesafezone")
		{
			auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
			auto GameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());

			UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), L"pausesafezone", nullptr);
			// GameMode->PauseSafeZone(GameState->IsSafeZonePaused() == 0);
		}
		else if (Command == "tp" || Command == "to" || Command == "teleport")
		{
			auto CheatManager = ReceivingController->SpawnCheatManager(UCheatManager::StaticClass());

			if (!CheatManager)
			{
				SendMessageToConsole(PlayerController, L"Failed to spawn player's cheat manager!");
				return;
			}

			CheatManager->Teleport();
			CheatManager = nullptr;
			SendMessageToConsole(PlayerController, L"Teleported!");
		}
		else if (Command == "destroytarget" || Command == "destroy")
		{
			auto CheatManager = ReceivingController->SpawnCheatManager(UCheatManager::StaticClass());

			if (!CheatManager)
			{
				SendMessageToConsole(PlayerController, L"Failed to spawn player's cheat manager!");
				return;
			}

			CheatManager->DestroyTarget();
			CheatManager = nullptr;
			SendMessageToConsole(PlayerController, L"Destroyed target!");
		}
		else if (Command == "bugitgo")
		{
			if (Arguments.size() <= 3)
			{
				SendMessageToConsole(PlayerController, L"Please provide X, Y, and Z!\n");
				return;
			}

			float X{}, Y{}, Z{};

			try { X = std::stof(Arguments[1]); }
			catch (...) {}
			try { Y = std::stof(Arguments[2]); }
			catch (...) {}
			try { Z = std::stof(Arguments[3]); }
			catch (...) {}

			auto Pawn = Cast<APawn>(ReceivingController->GetPawn());

			if (!Pawn)
			{
				SendMessageToConsole(PlayerController, L"No pawn to teleport!");
				return;
			}

			Pawn->TeleportTo(FVector(X, Y, Z), Pawn->GetActorRotation());
			SendMessageToConsole(PlayerController, L"Teleported!");
		}
		else if (Command == "settimeofday" || Command == "time" || Command == "hour")
		{
			static auto SetTimeOfDayFn = FindObject<UFunction>("/Script/FortniteGame.FortKismetLibrary.SetTimeOfDay");

			float NewTimeOfDay = 0.f;

			try { NewTimeOfDay = std::stoi(Arguments[1]); }
			catch (...) {}

			struct
			{
				UObject* WorldContextObject;
				float                              TimeOfDay;
			}params{ GetWorld() , NewTimeOfDay };

			UFortKismetLibrary::StaticClass()->ProcessEvent(SetTimeOfDayFn, &params);
		}
		else if (Command == "healthall")
		{
			for (int i = 0; i < ClientConnections.Num(); i++)
			{
				auto PlayerController = Cast<AFortPlayerController>(ClientConnections.at(i)->GetPlayerController());

				if (!PlayerController->IsValidLowLevel())
					continue;

				auto Pawn = PlayerController->GetMyFortPawn();

				Pawn->SetHealth(100.f);
			}

			SendMessageToConsole(PlayerController, L"Healed all players health!\n");
		}
		else if (Command == "shieldall")
		{
			for (int i = 0; i < ClientConnections.Num(); i++)
			{
				auto PlayerController = Cast<AFortPlayerController>(ClientConnections.at(i)->GetPlayerController());

				if (!PlayerController->IsValidLowLevel())
					continue;

				auto Pawn = PlayerController->GetMyFortPawn();

				Pawn->SetShield(100.f);
			}

			SendMessageToConsole(PlayerController, L"Healed all players shield!\n");
			}
		else if (Command == "godall")
		{
			for (int i = 0; i < ClientConnections.Num(); i++)
			{
				auto PlayerController = Cast<AFortPlayerController>(ClientConnections.at(i)->GetPlayerController());

				if (!PlayerController->IsValidLowLevel())
					continue;

				auto Pawn = PlayerController->GetMyFortPawn();
				Pawn->SetCanBeDamaged(!Pawn->CanBeDamaged());
				SendMessageToConsole(PlayerController, std::wstring(L"God of all players set to " + std::to_wstring(!(bool)Pawn->CanBeDamaged())).c_str());
			}
			}
		else if (Command == "grantall" || Command == "giveall")
		{
			static auto World_NetDriverOffset = GetWorld()->GetOffset("NetDriver");
			auto WorldNetDriver = GetWorld()->Get<UNetDriver*>(World_NetDriverOffset);
			auto& ClientConnections = WorldNetDriver->GetClientConnections();

			for (int z = 0; z < ClientConnections.Num(); z++)
			{
				auto ClientConnection = ClientConnections.at(z);
				auto FortPC = Cast<AFortPlayerController>(ClientConnection->GetPlayerController());

				if (!FortPC)
					continue;

				auto WorldInventory = FortPC->GetWorldInventory();

				if (!WorldInventory)
					continue;

				static auto WoodItemData = FindObject<UFortItemDefinition>(
					L"/Game/Items/ResourcePickups/WoodItemData.WoodItemData");
				static auto StoneItemData = FindObject<UFortItemDefinition>(
					L"/Game/Items/ResourcePickups/StoneItemData.StoneItemData");
				static auto MetalItemData = FindObject<UFortItemDefinition>(
					L"/Game/Items/ResourcePickups/MetalItemData.MetalItemData");
				static auto Gold = FindObject<UFortItemDefinition>(
					L"/Game/Items/ResourcePickups/Athena_WadsItemData.Athena_WadsItemData");
				static auto Crown = FindObject<UFortItemDefinition>(
					L"/VictoryCrownsGameplay/Items/AGID_VictoryCrown.AGID_VictoryCrown");

				static auto Sniper = FindObject<UFortItemDefinition>(
					L"");
				static auto Secondary = FindObject<UFortItemDefinition>(
					L"");
				static auto Tertiary = FindObject<UFortItemDefinition>(
					L"");
				static auto Consumable1 = FindObject<UFortItemDefinition>(
					L"");
				static auto Consumable2 = FindObject<UFortItemDefinition>(
					L"");

				static auto HeavyAmmo = FindObject<UFortItemDefinition>(
					L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsHeavy.AthenaAmmoDataBulletsHeavy");
				static auto ShellsAmmo = FindObject<UFortItemDefinition>(
					L"/Game/Athena/Items/Ammo/AthenaAmmoDataShells.AthenaAmmoDataShells");
				static auto MediumAmmo = FindObject<UFortItemDefinition>(
					L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsMedium.AthenaAmmoDataBulletsMedium");
				static auto LightAmmo = FindObject<UFortItemDefinition>(
					L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsLight.AthenaAmmoDataBulletsLight");
				static auto RocketAmmo = FindObject<UFortItemDefinition>(
					L"/Game/Athena/Items/Ammo/AmmoDataRockets.AmmoDataRockets");
				static auto ExplosiveAmmo = FindObject<UFortItemDefinition>(
					L"/Game/Items/Ammo/AmmoDataExplosive.AmmoDataExplosive");
				static auto EnergyCells = FindObject<UFortItemDefinition>(
					L"/Game/Items/Ammo/AmmoDataEnergyCell.AmmoDataEnergyCell");
				static auto Arrows = FindObject<UFortItemDefinition>(
					L"/PrimalGameplay/Items/Ammo/AthenaAmmoDataArrows.AthenaAmmoDataArrows");
				static auto ReconAmmo = FindObject<UFortItemDefinition>(
					L"/MotherGameplay/Items/Scooter/Ammo_Athena_Mother_Scooter.Ammo_Athena_Mother_Scooter");

				WorldInventory->AddItem(WoodItemData, nullptr, 999);
				WorldInventory->AddItem(StoneItemData, nullptr, 999);
				WorldInventory->AddItem(MetalItemData, nullptr, 999);
				WorldInventory->AddItem(Gold, nullptr, 10000);
				WorldInventory->AddItem(Sniper, nullptr, 1);
				WorldInventory->AddItem(Secondary, nullptr, 1);
				WorldInventory->AddItem(Tertiary, nullptr, 1);
				WorldInventory->AddItem(Consumable1, nullptr, 1);
				WorldInventory->AddItem(Consumable2, nullptr, 10);
				WorldInventory->AddItem(ShellsAmmo, nullptr, 999);
				WorldInventory->AddItem(HeavyAmmo, nullptr, 999);
				WorldInventory->AddItem(MediumAmmo, nullptr, 999);
				WorldInventory->AddItem(LightAmmo, nullptr, 999);
				WorldInventory->AddItem(RocketAmmo, nullptr, 999);
				WorldInventory->AddItem(ExplosiveAmmo, nullptr, 999);
				WorldInventory->AddItem(EnergyCells, nullptr, 999);
				WorldInventory->AddItem(Arrows, nullptr, 30);
				WorldInventory->AddItem(ReconAmmo, nullptr, 999);
				WorldInventory->AddItem(Crown, nullptr, 1);

				WorldInventory->Update();
				}
			}

		else { bSendHelpMessage = true; };
	}
	else { bSendHelpMessage = true; };

	if (bSendHelpMessage)
	{
		FString HelpMessage = LR"(
cheat giveitem <ShortWID> <Count=1> - Gives a weapon to the executing player, if inventory is full drops a pickup on the player.
cheat summon <BlueprintClassPathName> <Count=1> - Summons the specified blueprint class at the executing player's location. Note: There is a limit on the count.
cheat bugitgo <X> <Y> <Z> - Teleport to a location.
cheat launch/fling <X> <Y> <Z> - Launches a player.
cheat listplayers - Gives you all players names.
cheat pausesafezone - Pauses the zone.
cheat health <Health=100.f> - Sets executing player's health.
cheat shield <Shield=0.f> - Sets executing player's shield.
cheat spawnpickup <ShortWID> <ItemCount=1> <PickupCount=1> - Spawns a pickup at specified player.
cheat tp - Teleports to what the player is looking at.
cheat bot <Amount=1> - Spawns a bot at the player (experimental).
cheat pickaxe <PickaxeID> - Set player's pickaxe. Can be either the PID or WID.
cheat skin <CIDShortName> - Sets a player's character.
cheat destroy - Destroys the actor that the player is looking at.
cheat healthall - Heals all players health.
cheat shieldall - Heals all players shield.
cheat godall - Gods all players.
cheat giveall - Gives all players Ammo, Materials, and Traps maxed out.
cheat dbno - Knocks the player.
cheat getlocation - Gives you the current XYZ cords of where you are standing and copies them to your clipboard (useful for bugitgo)
cheat settimeofday <1-23> - Changes the time of day in game to a 24H time period.

If you want to execute a command on a certain player, surround their name (case sensitive) with \, and put the param anywhere. Example: cheat health \ralz\ 100
)";
		
		SendMessageToConsole(PlayerController, HelpMessage);
	}
}