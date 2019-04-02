import nuke

nuke.menu("Nodes").addMenu("Yannix")
 
nuke.menu("Nodes").addCommand( "Yannix/YnxRollingShutterNode", "nuke.createNode('YnxRollingShutterNode')" )
