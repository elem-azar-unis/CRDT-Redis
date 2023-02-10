-------------------------------- MODULE rmvwin_rpq --------------------------------
EXTENDS Integers, Sequences, TLC
CONSTANTS N, MaxOps, Values, Increments

Dft_Values == {10, 20}
Dft_Increments == {-3, 4}

Procs == 1..N

\* utilities

VC_Inc(vc, self) == [j \in Procs |-> IF j = self THEN vc[j] + 1 ELSE vc[j]]
VC_Max(a, b) == [j \in Procs |-> IF a[j] <= b[j] THEN b[j] ELSE a[j]]
VC_Less(a, b) == \A j \in Procs: a[j] <= b[j]
VC_Concurrent(a, b) == (\E i \in Procs: a[i] < b[i]) /\ (\E j \in Procs: a[j] > b[j])

Causally_Ready(msg, pid, local) == \A j \in Procs: (j = pid /\ msg[j] = local[j] + 1)
                                                 \/ (j /= pid /\ msg[j] <= local[j])

(*--algorithm rmvwin_rpq

variables
    ops = [j \in Procs |-> {}]; \* to broadcast operations
    opcount = 0;

define
    \* read operations
    _Lookup(a_set, r_set) == a_set /= {} /\ r_set = {}
    
    \* prepare phases
    _Add(a_set, r_set, self, local_vc, x) == 
        IF ~_Lookup(a_set, r_set) THEN
            [pid |-> self, t |-> VC_Inc(local_vc, self), val |-> x]
        ELSE [pid |-> -1]
    
    _Increase(a_set, r_set, self, local_vc, i) == 
        IF _Lookup(a_set, r_set) THEN 
            [pid |-> self, t |-> VC_Inc(local_vc, self), val |-> i]
        ELSE [pid |-> -1]
    
    _Remove(a_set, r_set, self, local_vc) == 
        IF _Lookup(a_set, r_set) THEN 
            [pid |-> self, t |-> VC_Inc(local_vc, self)]
        ELSE [pid |-> -1] 
end define;

\* send an operation to all
macro Broadcast(o, params) begin 
    ops := [j \in Procs |-> IF j = self THEN ops[j] 
                            ELSE ops[j] \union {[op |-> o, num |-> opcount, p |-> params]}];
end macro;


\* effect of add operation
macro Add(x, t, pid) begin
    a_set := (a_set \ {ar \in a_set: VC_Less(ar.t, t)}) \union {[t |-> t, pid |-> pid, v_inn |-> x, v_acq |-> 0]};
    r_set := r_set \ {rt \in r_set: VC_Less(rt, t)};
    local_vc := VC_Max(local_vc, t);
end macro;

\* effect of remove operation
macro Remove(t) begin
    r_set := r_set \union {t};
    a_set := a_set \ {ar \in a_set: VC_Less(ar.t, t)};
    local_vc := VC_Max(local_vc, t);
end macro;

\* effect of increase operation
macro Increase(i, t) begin
    with inc_ars = {ar \in a_set: VC_Less(ar.t, t)} do
        a_set := (a_set \ inc_ars) \union {[t |-> rcd.t, pid |-> rcd.pid, v_inn |-> rcd.v_inn, 
                             v_acq |-> rcd.v_acq + i]: rcd \in inc_ars};
    end with;
    local_vc := VC_Max(local_vc, t);
end macro;

\* receive and process operations, one by one
macro Update() begin 
    if ops[self] /= {} then
        with msg \in ops[self] do
            if Causally_Ready(msg.p.t, msg.p.pid, local_vc) then
                if msg.op = "A" then
                    Add(msg.p.val, msg.p.t, msg.p.pid);
                elsif msg.op = "R" then
                    Remove(msg.p.t);
                elsif msg.op = "I" then
                    Increase(msg.p.val, msg.p.t);
                end if;
                \* op_exed := op_exed \union {msg.num};
                ops[self] := ops[self] \ {msg}; \* clear processed operation
            end if;
        end with;
    end if;
end macro;

process Set \in Procs
variables
    a_set = {}; \* local set of pairs --
                \* [t |-> [], pid |-> "", v_inn |-> "", v_acq |-> ""] 
    r_set = {}; \* local set of t
    local_vc = [j \in Procs |-> 0];
    \* op_exed = {}; \* executed operations
begin Main:
    while TRUE do
        either
            if opcount < MaxOps then
                opcount := opcount + 1;
                either \* Add
                    with v \in Values, addp = _Add(a_set, r_set, self, local_vc, v) do \* select a random element-value to add
                        if addp.pid /= -1 then
                            Broadcast("A", addp);
                            Add(v, addp.t, self);
                            \* op_exed := op_exed \union {opcount};
                        end if;
                    end with;
                or \* Remove
                    with rmvp = _Remove(a_set, r_set, self, local_vc) do \* select a random element to remove
                        if rmvp.pid /= -1 then
                            Broadcast("R", rmvp);
                            Remove(rmvp.t);
                            \* op_exed := op_exed \union {opcount};
                        end if;
                    end with;
                or \* Increase
                    with i \in Increments, incp = _Increase(a_set, r_set, self, local_vc, i) do \* select a random element-increment to inc
                        if incp.pid /= -1 then
                            Broadcast("I", incp);
                            Increase(i, incp.t);
                            \* op_exed := op_exed \union {opcount};
                        end if;
                    end with;
                end either;
            end if;
        or Update();
        end either;
    end while;
end process;

end algorithm;*)
\* BEGIN TRANSLATION (chksum(pcal) = "b655ba5b" /\ chksum(tla) = "e0ce3af3")
VARIABLES ops, opcount

(* define statement *)
_Lookup(a_set, r_set) == a_set /= {} /\ r_set = {}


_Add(a_set, r_set, self, local_vc, x) ==
    IF ~_Lookup(a_set, r_set) THEN
        [pid |-> self, t |-> VC_Inc(local_vc, self), val |-> x]
    ELSE [pid |-> -1]

_Increase(a_set, r_set, self, local_vc, i) ==
    IF _Lookup(a_set, r_set) THEN
        [pid |-> self, t |-> VC_Inc(local_vc, self), val |-> i]
    ELSE [pid |-> -1]

_Remove(a_set, r_set, self, local_vc) ==
    IF _Lookup(a_set, r_set) THEN
        [pid |-> self, t |-> VC_Inc(local_vc, self)]
    ELSE [pid |-> -1]

VARIABLES a_set, r_set, local_vc

vars == << ops, opcount, a_set, r_set, local_vc >>

ProcSet == (Procs)

Init == (* Global variables *)
        /\ ops = [j \in Procs |-> {}]
        /\ opcount = 0
        (* Process Set *)
        /\ a_set = [self \in Procs |-> {}]
        /\ r_set = [self \in Procs |-> {}]
        /\ local_vc = [self \in Procs |-> [j \in Procs |-> 0]]

Set(self) == \/ /\ IF opcount < MaxOps
                      THEN /\ opcount' = opcount + 1
                           /\ \/ /\ \E v \in Values:
                                      LET addp == _Add(a_set[self], r_set[self], self, local_vc[self], v) IN
                                        IF addp.pid /= -1
                                           THEN /\ ops' = [j \in Procs |-> IF j = self THEN ops[j]
                                                                           ELSE ops[j] \union {[op |-> "A", num |-> opcount', p |-> addp]}]
                                                /\ a_set' = [a_set EXCEPT ![self] = (a_set[self] \ {ar \in a_set[self]: VC_Less(ar.t, (addp.t))}) \union {[t |-> (addp.t), pid |-> self, v_inn |-> v, v_acq |-> 0]}]
                                                /\ r_set' = [r_set EXCEPT ![self] = r_set[self] \ {rt \in r_set[self]: VC_Less(rt, (addp.t))}]
                                                /\ local_vc' = [local_vc EXCEPT ![self] = VC_Max(local_vc[self], (addp.t))]
                                           ELSE /\ TRUE
                                                /\ UNCHANGED << ops, a_set, 
                                                                r_set, 
                                                                local_vc >>
                              \/ /\ LET rmvp == _Remove(a_set[self], r_set[self], self, local_vc[self]) IN
                                      IF rmvp.pid /= -1
                                         THEN /\ ops' = [j \in Procs |-> IF j = self THEN ops[j]
                                                                         ELSE ops[j] \union {[op |-> "R", num |-> opcount', p |-> rmvp]}]
                                              /\ r_set' = [r_set EXCEPT ![self] = r_set[self] \union {(rmvp.t)}]
                                              /\ a_set' = [a_set EXCEPT ![self] = a_set[self] \ {ar \in a_set[self]: VC_Less(ar.t, (rmvp.t))}]
                                              /\ local_vc' = [local_vc EXCEPT ![self] = VC_Max(local_vc[self], (rmvp.t))]
                                         ELSE /\ TRUE
                                              /\ UNCHANGED << ops, a_set, 
                                                              r_set, 
                                                              local_vc >>
                              \/ /\ \E i \in Increments:
                                      LET incp == _Increase(a_set[self], r_set[self], self, local_vc[self], i) IN
                                        IF incp.pid /= -1
                                           THEN /\ ops' = [j \in Procs |-> IF j = self THEN ops[j]
                                                                           ELSE ops[j] \union {[op |-> "I", num |-> opcount', p |-> incp]}]
                                                /\ LET inc_ars == {ar \in a_set[self]: VC_Less(ar.t, (incp.t))} IN
                                                     a_set' = [a_set EXCEPT ![self] = (a_set[self] \ inc_ars) \union {[t |-> rcd.t, pid |-> rcd.pid, v_inn |-> rcd.v_inn,
                                                                                                        v_acq |-> rcd.v_acq + i]: rcd \in inc_ars}]
                                                /\ local_vc' = [local_vc EXCEPT ![self] = VC_Max(local_vc[self], (incp.t))]
                                           ELSE /\ TRUE
                                                /\ UNCHANGED << ops, a_set, 
                                                                local_vc >>
                                 /\ r_set' = r_set
                      ELSE /\ TRUE
                           /\ UNCHANGED << ops, opcount, a_set, r_set, 
                                           local_vc >>
             \/ /\ IF ops[self] /= {}
                      THEN /\ \E msg \in ops[self]:
                                IF Causally_Ready(msg.p.t, msg.p.pid, local_vc[self])
                                   THEN /\ IF msg.op = "A"
                                              THEN /\ a_set' = [a_set EXCEPT ![self] = (a_set[self] \ {ar \in a_set[self]: VC_Less(ar.t, (msg.p.t))}) \union {[t |-> (msg.p.t), pid |-> (msg.p.pid), v_inn |-> (msg.p.val), v_acq |-> 0]}]
                                                   /\ r_set' = [r_set EXCEPT ![self] = r_set[self] \ {rt \in r_set[self]: VC_Less(rt, (msg.p.t))}]
                                                   /\ local_vc' = [local_vc EXCEPT ![self] = VC_Max(local_vc[self], (msg.p.t))]
                                              ELSE /\ IF msg.op = "R"
                                                         THEN /\ r_set' = [r_set EXCEPT ![self] = r_set[self] \union {(msg.p.t)}]
                                                              /\ a_set' = [a_set EXCEPT ![self] = a_set[self] \ {ar \in a_set[self]: VC_Less(ar.t, (msg.p.t))}]
                                                              /\ local_vc' = [local_vc EXCEPT ![self] = VC_Max(local_vc[self], (msg.p.t))]
                                                         ELSE /\ IF msg.op = "I"
                                                                    THEN /\ LET inc_ars == {ar \in a_set[self]: VC_Less(ar.t, (msg.p.t))} IN
                                                                              a_set' = [a_set EXCEPT ![self] = (a_set[self] \ inc_ars) \union {[t |-> rcd.t, pid |-> rcd.pid, v_inn |-> rcd.v_inn,
                                                                                                                                 v_acq |-> rcd.v_acq + (msg.p.val)]: rcd \in inc_ars}]
                                                                         /\ local_vc' = [local_vc EXCEPT ![self] = VC_Max(local_vc[self], (msg.p.t))]
                                                                    ELSE /\ TRUE
                                                                         /\ UNCHANGED << a_set, 
                                                                                         local_vc >>
                                                              /\ r_set' = r_set
                                        /\ ops' = [ops EXCEPT ![self] = ops[self] \ {msg}]
                                   ELSE /\ TRUE
                                        /\ UNCHANGED << ops, a_set, r_set, 
                                                        local_vc >>
                      ELSE /\ TRUE
                           /\ UNCHANGED << ops, a_set, r_set, local_vc >>
                /\ UNCHANGED opcount

Next == (\E self \in Procs: Set(self))

Spec == Init /\ [][Next]_vars

\* END TRANSLATION 

AA_Con == \A rcd1, rcd2 \in a_set[1]:
                        rcd1.t /= rcd2.t => VC_Concurrent(rcd1.t, rcd2.t)

RR_Con == \A t1, t2 \in r_set[1]:
                        t1 /= t2 => VC_Concurrent(t1, t2)

AR_Con == \A rcd \in a_set[1]: \A t \in r_set[1]: VC_Concurrent(rcd.t, t)

Rcd_Concurrent == AA_Con /\ AR_Con /\ RR_Con

SEC == \A p1, p2 \in Procs: (p1 /= p2 /\ ops[p1] = ops[p2]) => 
                            (a_set[p1] = a_set[p2] /\ r_set[p1] = r_set[p2])

================================================================================
