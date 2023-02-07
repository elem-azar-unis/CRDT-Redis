# TLA+ specs for CRDTs

Here are the TLA+ specifications for Add-Win CRPQ, Remove-Win CRPQ, RWF-CRPQ and RWF-List. They are used to check the correctness of the design of the CRDTs.

To check the CRDT design, create and run TLA models in the usual way. We check the *Strong Eventual Consistency* for CRDTs, and additionally *elements are always total ordered* for RWF-List.

Additionally the specs of RWF-CRPQ and RWF-List are used for generating the test cases to test the CRDT implementations.
To generate test cases, use ```-userFile``` option to run TLA models. This will write the states we need to a file. Make sure that the checking is successful. Parse the output file with *paese.py*. This will generate the test case file, which will be further used for [testing the CRDT implementation in the system](../).
