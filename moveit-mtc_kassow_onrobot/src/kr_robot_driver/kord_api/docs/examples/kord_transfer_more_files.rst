Transfer Multiple Files at once
===============================

.. warning::
    Do not attempt to retrieve files while robot is moving. Also do not 
    attempt to initiate robot movement when robot is transferring data.

Setup
-----

To transfer the files, some fields of the ``KORD.ini`` can be changed and ssh key must be added to the host.
See :doc:`../configuration/kord_ini` for more information.

Usage
-----

.. code-block::

 $ ./kord_transfer_more_files -h
    [OPTIONS]
     -h               show this help
     -r <prio>        execute as a realtime process with priority set to <prio>
     -p <port>        port number to connect to
     -c <IP Address>  remote controller IP address
     -i <Session ID>  KORD session ID | Default: 1

Trouble shooting
----------------

The output could be either **SUCCESS** or **FAIL**. 
In latter case the error code is also returned.

=====  ===========
Error  Description
=====  ===========
1      If this happens, please, notify the support about it.
2      | The create/upload command failed. 
       | Hints:

       * be sure that `KORD.ini` file is uploaded to teach pendant,
       * be sure that keys are exchanged.

3      | The `KORD.ini` file is corrupted or missing.
       | Hints:

       * be sure that `KORD.ini` file is uploaded to teach pendant

4,5    | The host's username is corrupted or missing.
       | Hints:
       
       * be sure that `KORD.ini` file has the proper structure. (see example above)

7      The uploading is in the progress. Try latter.
=====  ===========
