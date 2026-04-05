#!/usr/bin/env python
"""
SoccerSim Material Creation Script
Creates football and pitch materials with proper textures
Run this in Unreal Editor's Python console
"""

import unreal
import os

def create_football_material():
    """Create a realistic football material"""
    
    # Material path
    material_path = "/Game/Art/Ball/M_Football"
    
    # Create material if it doesn't exist
    if unreal.EditorAssetLibrary.does_asset_exist(material_path):
        print(f"Material already exists: {material_path}")
        return
    
    # Create material
    material_factory = unreal.MaterialFactoryNew()
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_Football",
        "/Game/Art/Ball",
        unreal.Material,
        material_factory
    )
    
    if material:
        print(f"Created material: {material_path}")
        
        # Set up basic material properties
        # Note: Full material setup requires node graph manipulation
        # which is complex via Python. This creates the base material.
        
        # Save the material
        unreal.EditorAssetLibrary.save_loaded_asset(material)
        print("Material saved!")
    else:
        print("Failed to create material")

def create_pitch_material():
    """Create a realistic grass pitch material"""
    
    material_path = "/Game/Art/Pitch/M_Grass"
    
    if unreal.EditorAssetLibrary.does_asset_exist(material_path):
        print(f"Material already exists: {material_path}")
        return
    
    material_factory = unreal.MaterialFactoryNew()
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_Grass",
        "/Game/Art/Pitch",
        unreal.Material,
        material_factory
    )
    
    if material:
        print(f"Created material: {material_path}")
        unreal.EditorAssetLibrary.save_loaded_asset(material)
        print("Material saved!")
    else:
        print("Failed to create material")

def create_goal_net_material():
    """Create a realistic goal net material"""
    
    material_path = "/Game/Art/Goals/M_GoalNet"
    
    if unreal.EditorAssetLibrary.does_asset_exist(material_path):
        print(f"Material already exists: {material_path}")
        return
    
    material_factory = unreal.MaterialFactoryNew()
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_GoalNet",
        "/Game/Art/Goals",
        unreal.Material,
        material_factory
    )
    
    if material:
        print(f"Created material: {material_path}")
        unreal.EditorAssetLibrary.save_loaded_asset(material)
        print("Material saved!")
    else:
        print("Failed to create material")

def main():
    """Create all materials"""
    print("=== Creating SoccerSim Materials ===")
    
    # Create folders
    folders = [
        "/Game/Art",
        "/Game/Art/Ball",
        "/Game/Art/Pitch", 
        "/Game/Art/Goals",
        "/Game/Art/Stadium",
        "/Game/Art/Particles",
        "/Game/Art/UI"
    ]
    
    for folder in folders:
        unreal.EditorAssetLibrary.make_directory(folder)
        print(f"Folder ready: {folder}")
    
    # Create materials
    create_football_material()
    create_pitch_material()
    create_goal_net_material()
    
    print("=== Materials Created ===")
    print("Note: Materials are empty shells. Connect nodes in Material Editor.")
    print("For football: Add BaseColor (white with black pentagon pattern)")
    print("For grass: Add BaseColor (green), Normal, Roughness")
    print("For net: Add opacity/translucency settings")

if __name__ == "__main__":
    main()
