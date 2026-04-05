Debug an issue in the running PIE session. Use this when something looks wrong (ball clipping, players not moving, visual glitches).

1. Check PIE state: `MSYS_NO_PATHCONV=1 soft-ue-cli pie-session get-state`
2. Capture viewport screenshot: `soft-ue-cli capture-viewport`
3. Read the screenshot with the Read tool to visually inspect
4. Check logs for the issue: `soft-ue-cli get-logs --lines 50 --filter error` or `--filter LogSoccerSim`
5. Query relevant actors via Python script (adapt based on the issue):
   - Ball: `unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoccerBall)`
   - Players: `unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoccerPlayerPawn)`
   - All actors: `soft-ue-cli query-level --world pie`
6. Report diagnosis with root cause analysis (follow Superpowers systematic-debugging discipline)

Arguments: $ARGUMENTS — describe what's wrong (e.g., "ball falling through floor", "players invisible", "no HUD")
