import unreal
es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = es.get_game_world()
if not world:
    print("No PIE world")
else:
    balls = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoccerBall)
    players = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoccerPlayerPawn)
    print(f"Players: {len(players)}, Balls: {len(balls)}")

    if balls:
        ball = balls[0]
        print(f"Ball: loc={ball.get_actor_location()}, vel={ball.get_velocity()}")

    moving = 0
    for i, p in enumerate(players[:6]):
        loc = p.get_actor_location()
        vel = p.get_velocity()
        speed = (vel.x**2 + vel.y**2 + vel.z**2)**0.5
        name = p.get_name()
        if speed > 10.0:
            moving += 1
        print(f"  {name}: loc=({loc.x:.0f},{loc.y:.0f},{loc.z:.0f}) speed={speed:.0f}")
    print(f"Moving players (of 6 checked): {moving}")
