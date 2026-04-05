#!/usr/bin/env python
"""
SoccerSim Animation Import Script
Run this in Unreal Editor's Python console
"""

import unreal
import os

# Paths
FBX_SOURCE_DIR = r"C:\Users\byron\.openclaw\Soccer"
DEST_FOLDER = "/Game/Anim/Mixamo"

def import_animations():
    """Import all Mixamo FBX animations"""
    
    # Create destination folder if it doesn't exist
    unreal.EditorAssetLibrary.make_directory(DEST_FOLDER)
    
    # Get all FBX files
    fbx_files = [f for f in os.listdir(FBX_SOURCE_DIR) if f.endswith('.fbx')]
    
    print(f"Found {len(fbx_files)} FBX files to import")
    
    # Import each FBX
    for fbx_file in fbx_files:
        source_path = os.path.join(FBX_SOURCE_DIR, fbx_file)
        
        # Set up import options for animation
        import_task = unreal.AssetImportTask()
        import_task.filename = source_path
        import_task.destination_path = DEST_FOLDER
        import_task.automated = True
        import_task.save = True
        
        # Configure FBX import options
        import_options = unreal.FbxImportUI()
        import_options.import_mesh = False  # Don't import mesh, just animation
        import_options.import_as_skeletal = True
        import_options.import_animations = True
        import_options.override_full_name = True
        
        import_task.options = import_options
        
        # Execute import
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([import_task])
        
        print(f"Imported: {fbx_file}")
    
    print(f"Successfully imported {len(fbx_files)} animations to {DEST_FOLDER}")

if __name__ == "__main__":
    import_animations()
