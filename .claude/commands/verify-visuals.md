Run the full visual verification protocol for SoccerSim. Do ALL steps before reporting results:

1. Check PIE is running: `soft-ue-cli pie-session get-state` (use MSYS_NO_PATHCONV=1)
   - If not running, say "PIE not running — start it first" and stop
2. Capture viewport screenshot: `soft-ue-cli capture-viewport`
3. Read the screenshot PNG file with the Read tool (supports image viewing)
4. Query ball position via Python script:
   ```
   soft-ue-cli run-python-script --script "import unreal; es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem); world = es.get_game_world(); actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoccerBall); [print(f'Ball: loc=({a.get_actor_location().x:.1f}, {a.get_actor_location().y:.1f}, {a.get_actor_location().z:.1f}), speed={a.get_velocity().length():.1f}') for a in actors]"
   ```
5. Query player positions:
   ```
   soft-ue-cli run-python-script --script "import unreal; es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem); world = es.get_game_world(); players = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoccerPlayerPawn); home = sum(1 for p in players if p.get_editor_property('team_id') == unreal.TeamId.HOME); away = sum(1 for p in players if p.get_editor_property('team_id') == unreal.TeamId.AWAY); print(f'Home: {home}, Away: {away}')"
   ```
6. Check for errors: `soft-ue-cli get-logs --filter error`

Report findings as a structured summary. Flag any issues (ball underground, missing players, errors).
