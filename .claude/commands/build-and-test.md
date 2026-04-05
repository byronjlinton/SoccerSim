Build SoccerSim, launch the editor, wait for PIE, and run visual verification.

1. Build the project:
   ```bash
   "/c/Program Files/Epic Games/Unreal 2/UE_5.7/Engine/Build/BatchFiles/Build.bat" \
     SoccerSimEditor Win64 Development "c:/Users/byron/Cursor/SoccerSim/SoccerSim.uproject" -waitmutex
   ```
   - If build fails, report errors and stop.

2. Kill any existing editor: `taskkill //F //IM UnrealEditor.exe`

3. Launch editor (run in background):
   ```bash
   "/c/Program Files/Epic Games/Unreal 2/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe" \
     "c:/Users/byron/Cursor/SoccerSim/SoccerSim.uproject"
   ```

4. Wait 30-40 seconds for editor + PIE to start (PIE auto-starts from DefaultEngine.ini)

5. Run `/verify-visuals` to confirm everything is working

Report build status, editor launch status, and verification results.
