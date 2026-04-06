import unreal
es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = es.get_game_world()
players = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoccerPlayerPawn)
ball = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoccerBall)
speeds = [p.get_velocity().size() for p in players]
print(f"Players: {len(players)}")
moving = [i for i, enumerate(speeds) if i > 50]
print(f"Moving players: {len(moving)}")
for i, range(0, len(players)):
    print(f"  P{i}: {speeds[i]:.0f} cm/s")
print(f"Ball vel: {ball[0].get_velocity().size():.1f} cm/s" if ball else "0No ball")
