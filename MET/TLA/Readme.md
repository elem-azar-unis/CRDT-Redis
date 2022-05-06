# TLA+ specs for CRDTs

Here are the TLA+ specifications for RWF-RPQ and RWF-List. The purposes for the TLA+ specs are:

1. Check the correctness of the design of the CRDTs.
2. Generate the test cases to test the CRDT implementations.

To check the CRDT design, create and run TLA models in the usual way.

To generate test cases, use ```-userFile``` option to run TLA models. This will write the states we need to a file. Make sure that the checking is successful. Parse the output file with *paese.py*. This will generate the test case file, which will be further used for [testing the CRDT implementation in the system](../).
