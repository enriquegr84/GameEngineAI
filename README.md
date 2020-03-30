# Game Engine AI

Game engine A.I. is a personal research project aimed to find a challenging system that we can apply on action games, 
particularly for shooters. In the wiki section, we analyze the current state of the art of AI techniques that have been 
successfully applied on videogames and introduce a new approach that has been implemented on a modified version of the 
Quake 3 Arena using the following framework https://github.com/enriquegr84/GameEngineTutorial. This new approach has been 
separated in the following main development phases: 

-	Modeling phase. We define the data structures necessary to represent a discrete approximation of the virtual world and 
  implement algorithms that create the discrete world via physics simulation.
-	Decision-making phase. We implement a runtime minimax algorithm for an NPC duel combat that simultaneously simulates both 
  players actions, and the NPC basic behavior to carry out the decision-making plans.  
-	Challenging phase. We propose a system that replace the most optimal heuristics of the decision-making algorithm with the 
  most challenging heuristics based on the player skills. The player skills are studied using a less constrained minimax 
  search tree and are represented numerically as statistics.

Unfortunately, we couldn’t get permission to upload the required Quake assets which means that the game can’t be executed but
the documentation comes with videos that show what the A.I. is capable of.
