
TO COMPILE:
  ccmake .
  # select devices in gui
  make
You'll need to get any extra software needed for each device, or remove
it from this directory.

=======================================================================

stage:
  Driver for a robot in the stage simulator (of player/stage fame).
  example invocation:
	./yarpy --device stage --world empty.world --robot robot1

=======================================================================

fakebot:
  A simple simulated robot head.
  example invocation:
	./yarpy --file fakebot.ini

