# SoccerSim Scripts

**To run the game with no manual level setup:** run `Scripts/EnsureSoccerSimDefaults.ps1`, restart the editor if it was open, then open the project and press Play.

**Clean restart (close editor and reopen project):** Run `Scripts\Restart-SoccerSim.ps1` from the project root (or from `SoccerSim\Scripts\`). This closes any running Unreal Editor process and launches `SoccerSim.uproject`. Use this after changing `Config/DefaultGame.ini` (e.g. MetaHuman path) so the editor loads fresh config. Save your work in the editor before running, or close the editor manually first if you prefer.
