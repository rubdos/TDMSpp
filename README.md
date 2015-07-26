TDMSpp
======

Cross-platform module for reading TDMS files as produced by LabView.

It is written for speeding up [npTDMS](https://github.com/adamreeve/npTDMS).
For more information, see the [npTDMS documentation](http://readthedocs.org/docs/nptdms).

Installation/usage
------------------

TODO

Links
-----

Source code lives at https://github.com/rubdos/TDMSpp and any issues can be
reported at https://github.com/rubdos/TDMSpp/issues.
Documentation for npTDMS is available at http://readthedocs.org/docs/nptdms.

What Currently Doesn't Work
---------------------------

This module doesn't support TDMS files with XML headers or with
extended floating point data.

Files with DAQmx raw data are not supported either.

Contributors/Thanks
-------------------

Thanks to Adam Reeve for writing npTDMS.

Thanks to Floris van Vugt who wrote the pyTDMS module,
which helped when writing the npTDMS module, of which TDMSpp
is heavily based.
