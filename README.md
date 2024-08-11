# Robot Summer: ENPH253

Team Members: Brianna Gopaul, Ella Yan, Tristan Brown, Nathan Tourveille 

## Competition Outline
(in progress) 

In 6 weeks, we were tasked to build 2 autonomous robots that can play the game Overcooked, in real life. The goal of the competition is to earn the most points by making as many meal combinations as possible in 2 minutes. Each robot must be able to drive to ingredient areas, pick up individual ingredients and stack them to make burgers, salads and other meal combinations each worth a different amount of points. Finally, the robot must serve the meal on a plate at the serving area where an 1khz IR beacon is located. 

<img width="600" alt="competition-surface" src="https://github.com/user-attachments/assets/4a3e769f-0b4e-4c0a-86f9-7d27019f055e">

Our strategy was to make the most burgers possible in 2 minutes. Our first robot is located on the top region of the competition surface. It's designed to pick up ingredients and place them on the cooking surface for the second robot to pick up. The second robot picks up ingredients and stacks them in a chute. This chute vertically aligns the ingredients to make burgers. Once all the ingredients have been placed in the chute, the robot dispenses the ingredients on a plate. 

### PCBs 
(Brianna)
#### H Bridge
Since premade H Bridges were not permitted, we made H Bridge PCBs.  

<img src="https://github.com/user-attachments/assets/f49de4b8-fa06-483f-acd4-e2d75fc72247" alt="IMG_0039" width="300">

#### IR Beacon Detection PCBs

<img src="https://github.com/user-attachments/assets/d81d688a-c8bc-42c8-bca0-25481b5ce81f" alt="IMG_0043" width="300">

This V2 IR Sensing PCB has a bandpass filter and a peak detect circuit to filter out irrelevant IR frequencies. This was added because we decided that writing cross-correlation software was a less interesting solution. On our robot, we have 2 IR phototransistors angled 45 degrees away from each other. When approaching the serving area, we compare the amplitudes of the signals from the left and right phototransistors and approach the direction with the highest amplitude signal.

<img src="https://github.com/user-attachments/assets/2cbc61e7-e693-4115-9b50-94a7f64ad115" alt="FullSizeRender" width="300">

#### Tape Following PCB
JST connectors were used so that faulty TCRTs could be easily swapped out. 

<img src="https://github.com/user-attachments/assets/88dda804-2ab3-4c72-81c2-b679db210f3b" alt="IMG_8871" width="300">

#### Power

#### Mechanical Design

##Claw Design
Both claws were designed to scoop the food from underneath. To do this we used thin plastic on the lips of the claw which enabled the claw to fully be in contact with the ground even if we are not flush with the ground. After grabbing the food, the claw would rotate back and the food would slide down into the cup dropping mechanism

##Cup Design
Once the burger is assembled in the cup, a servo would move the cup over the plate and another servo would open the bottom of the cup and the burger would drop.

####Software
##Line Following
A decent bit of software was related to the TCRT readings. We just used a proportional constant to line follow and we would use the side TCRTs to detect when we were at stations. From there we then backtrack if we ever overshot and we would have aligning software if the robot was ever not parallel with the black line.

##Servos
Another bit of software was writing all the functions where the servos would be actuated. For example we had a grabFood and grabPLate function and many more. Combining these functions with the line detection software was essentially the essence of the code.



