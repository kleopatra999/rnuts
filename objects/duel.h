
#define CHAL_LINE "\n\n~OLHey you!  You have been challenged to a duel.\n"
#define CHAL_LINE2 "\n~OLRespond with: ~FY.duel [yes | no]"
#define CHAL_ISSUED "\n~OL~FR## ~FWThe challenge has been made...wait for a response... ~FR##\n"

UR_OBJECT fighter[2];        /* Contains the fighters. */

char chal_text[][132] = {
  "\n%s has challenged %s to a duel.  Call the medics!\n",
  "\n%s takes a leather glove and slaps %s in the face...its a challenge to duel.\n",
  "\n%s pulls out a weapon and says \"%s, make my day\".\n",
  "\n%s has had enough of %s...time to put on the gloves and settle this.\n",
  "\n%s shouts \"%s, you piece of scum, prepare to die.\"\n",
  "\n%s sucker punches %s.  Oh my...looks like a duel is starting.\n",
  "\n%s draws a line in the dirt and says: \"%s, go ahead, cross me.\"\n",
  "\n%s and %s square off.  They look each in the eyes.  A SHOWDOWN, LOOK OUT\n",
  "\n%s grabs %s and says \"HEY Shorty, I'll knock your block off.\"\n",
  "\n%s asks %s, \"umm... you wanna step outside...Mister?!\"\n",
  "\n%s shouts \"%s's mother wears army boots!\"  Oh NO....a duel!\n",
  "\n%s just dropped a spider down %s's pants....looks like a duel is about to start.\n",
  "\n%s pokes %s and says \"I could kick your butt with one hand behind my back!\"\n"
  };
int num_chal_text = 13;

char tie1_text[][132] = {
"\nThere is a mutual agreement struck and both %s and %s end the duel winners.\n",
"\nThe crowd restrains both people and they cool off...It is over people, go home.\n",
"\nPSYCHE...Y'all thought we were dueling didn't you. %s shakes %s hand. No way.\n",
"\nIn a flash, both %s and %s knock each other out.  They awake forgetting to finish it.\n",
"\n%s and %s make up and decide to open a weapons shop together instead.\n",
"\n%s cracks a joke and they both start laughing so hard...they cannot duel.\n"
};
int num_tie1_text = 6;

char tie2_text[][132] = {
"\nWow! They killed each other at exactly the same time. How sad. \n",
"\nBoth %s and %s die....seems they got stuck together with crazy glue in the duel.\n",
"\n%s killed %s, but was lynched by a mob of on-lookers....the irony of it all.\n",
"\nA run-away bus runs over %s and %s while they were dueling in the street.  OOPS.\n",
"\n%s decides to go suicidal and pulls the pin on the gernade, killing %s as well.\n",
"\n%s and %s are mistaken as ~FRBLOODS ~FWby a group of ~FBCRYPTS~FW...and are shot in a drive-by.\n",
"\nA plane drops out of the sky on top of both %s and %s killing them dead.  What luck.\n"
};
int num_tie2_text = 7;

char wins1_text[][132] = {
"\n%s is quick on the draw and pummels %s into the ground.\n",
"\n%s demonstrates expertise in martial arts and quickly destroys %s.\n",
"\n%s's initial attack must have been enough to allow for a fast kill of %s.\n",
"\n%s whips out a hand gernade and blasts %s into the next universe.\n",
"\n%s grabs 2 stone paddles and cracks %s head (like in the book Congo).\n",
"\n%s whips out an assault rifle and shoots %s dead.\n",
"\n%s was cleaning a gun and it went off 'accidentally', killing %s (oops).\n",
"\n%s shows %s some pet pirranhas ..... the rest is history.\n",
"\n%s hires a hitman and has %s killed....but it LOOKS like an accident.\n",
"\n%s's flesh melts away revealing a robot. The robot kills %s.\n",
"\n%s waits until %s is asleep.....and places a mad rattle snake in the bed.\n",
"\n%s ducks just in time to get missed by the sniper that was on the roof.  %s got it instead.\n",
"\n%s starts talking about gross stuff...%s had to run to the bathroom to puke.\n",
"\n%s pulls the plug on %s's computer.....tsk tsk tsk...no power, no net.\n",
"\n%s points and says \"Don't Look!\".  %s turns to look and is killed.  Told you not to look.\n" 
};
int num_wins1_text = 15;

char wins2_text[][132] = {
"\n%s accepts the challange to duel and pummels %s...so much for sneak attacks.\n",
"\n%s turns into a raving maniac and scares %s to death....good job.\n",
"\n%s shows %s a mirror.  Death was the result (embarrassment i think). \n",
"\n%s kicks the crap out of %s....who now has to leave to goto the hospital.\n",
"\n%s jumps in a car and runs %s over....Road Pizza!!!\n",
"\n%s throws a banana peel under %s's foot....who then slips, falls, and dies.\n",
"\n%s takes %s swimming (who was just fitted with cement shoes).\n",
"\n%s scares %s bad...they have an accident and have to leave to get new clothes.\n",
"\n%s calls the cops.  They come and take %s away for threatening people.\n",
"\n%s summons unearthly forces and vaporizes %s.....Wow, get the popcorn.\n",
"\n%s forces %s to watch a Barney marathon... the result is not pretty.\n",
"\n%s placed %s on a hook to be fish bait.\n",
"\n%s grabs a steam roller and squishes %s like a pancake.\n",
"\n%s drops an anvil on top of %s head.  OUCH.  That even looked like it hurt.\n",
"\n%s watches %s consume two beers and pass out. (cannot handle the stuff eh?)\n"
};
int num_wins2_text =15;

char wimp_text[][132] = {
"\n%s wimps out of the duel.  What a wuss.\n",
"\nHEY LOOK!  %s is a total chicken and has backed out of the duel.\n",
"\n%s declines the duel...maybe some other day when i feel better.\n",
"\nThe duel has been cancelled, %s had to go to assertiveness training.\n",
"\n%s got scared and called the police.  No duel today.\n",
"\n%s didn't want to duel....must have been cowardice.\n",
"\n%s turns yellow with fear and begs forgiveness. (OK..no duel)\n",
"\n%s runs with their tail between thier legs...guess that means no duel.\n",
"\n%s does a fake ignore of the challenge to the duel...hoping no one sees it.\n",
"\n%s suddenly remembers a dental appointment...\n"
};
int num_wimp_text = 10;

char kill_text[][100] = 
  {
  "\nA stream of electrons pulse through %s and turn them to vapor!!\n",
  "\n%s is destroyed by a wizard among us!!\n",
  "\nA large laser beam hits %s!!  Good bye..you're dead.\n",
  "\nSomeone cleaning their magic wand accidentaly kills %s!!  OOPS!\n",
  "\nA hiding net monster jumps out and eats %s!!  Too bad!\n",
  "\n%s had to leave....someone did not like them!!\n",
  "\n%s had to go to incontenence training!!  Bye Bye.\n",
  "\n%s had an important meeting...with DEATH!\n",
  "\n%s was just put out of our misery!\n",
  "\nA 2 ton weight just fell on %s!! Too bad this aint a cartoon.\n",
  "\n%s just net.sexed themself to death!! Will you be next?\n",
  "\nA huge shark emerges from the ocean to swallow %s!!\n",
  "\nThe omnipotent wizard has grown tired of %s's presence! Poof!\n",
  "\n%s's bytes are suddenly scrambled!\n",
  "\nWOW! %s had to go get gussied up in a pink dress!  How CUTE!\n",
  "\nA bolt of lightening streaks from the heavens and blasts %s!!\n"
  };
int num_kill_text = 16;

char brawl_text[][132] =
  {
  "\n%s shouts: \"Who in here feels they have ~FYlived ~FWlong enough??!!\"\n",
  "\n%s says, \"I'm bored.....let's ~FR~OLDANCE!!!!\"\n",
  "\n\"You're all a bunch of ~FTWIMPS!\"~FW, says %s.  Anyone care to prove them wrong?\n",
  "\n%s feels invulnerable today.  Anyone feel the same?\n",
  "\n%s shouts: \"~OLLET'S GET READY TO ~FRRUMBLLLLLLLLLLLLLLLLLLE~FW!!!!!\"\n",
  };
int num_brawl_text = 5;
