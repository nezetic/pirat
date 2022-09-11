/**
 * Ported from Valdemar Erlingsson original C# implementation.
 *
 * https://github.com/ValdemarOrn/SharpSoundPlugins/tree/master/Rodent.V2
 */
#include "Splines.h"

using namespace prat;

const std::array<std::array<float, Splines::D1N914TF_len>, 3> Splines::D1N914TF = {{
    {{-11.92876297296054,-7.9891391168010966,-1.4811025709312014,-0.7081535605041932,-0.21418063689620054,0.52772395496592028,1.1154141751797377,3.3186095107730171,7.9966744877839853,10.15950873895731}},
    {{-0.68727886215457579,-0.67766459383185929,-0.580964089609628,-0.51207665124988844,-0.23156585781066089,0.46578004319844568,0.56007100052876868,0.632169338001374,0.67816987691562369,0.68071606629320613}},
    {{0.0,0.004909814719624526,0.036172308444580692,0.13717421124828522,0.86663239816392912,0.32555077600122623,0.062625187750312825,0.01693385076768459,0.004909814719624526,0.0}}
}};

const std::array<std::array<float, Splines::LEDTF_len>, 3> Splines::LEDTF = {{
    {{-20.007141638482718,-2.915444438229212,-2.3941616350616366,-1.8681638548866069,-1.4632915735272494,1.2229924586554555,1.572843491744981,2.0432013992943152,2.8757611034829695,8.2512655010381142,20.015064279299438}},
    {{-1.731330241713267,-1.6225464128179352,-1.5972687114719562,-1.5391672822166931,-1.3508784208741995,1.1901803870295045,1.4297180755143659,1.5585133996818914,1.6157252109612972,1.6836420515583583,1.7312785241335889}},
    {{0.0,0.01693385076768459,0.053005958911864777,0.18527035544052548,0.69027986945904818,0.79368657947236787,0.59408758107456761,0.13717421124828522,0.028957886815744655,0.004909814719624526,0.0}}
}};