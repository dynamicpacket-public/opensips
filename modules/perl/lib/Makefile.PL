#!/usr/bin/perl

use strict;
use ExtUtils::MakeMaker;

system qq|cp -r perl lib|;

WriteMakefile(
        NAME            => 'OpenSIPS',
        VERSION_FROM    => 'perl/OpenSIPS.pm',
);

