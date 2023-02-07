-------------------------------- MODULE addwin_rpq --------------------------------
EXTENDS Integers, Sequences, TLC
CONSTANTS N, MaxOps, Elmts, Values, Increments

Dft_Values == {10, 20}
Dft_Increments == {-3, 4}
Dft_Elmts == {"e"}

Procs == 1..N 

(*--algorithm addwin_rpq

variables
    ops = [j \in Procs |-> {}]; \* to broadcast operations
    opcount = 0;

define
    Abs(x) == IF x < 0 THEN -x ELSE x
    \* read operations
    _T_Set(set, e) == {s.t: s \in {s \in set: s.key = e}}
    _Observe_T(a_set, r_set, e) == _T_Set(a_set, e) \ _T_Set(r_set, e)
    _Lookup(a_set, r_set, e) == _Observe_T(a_set, r_set, e) /= {}
    _Element(a_set, e, t) == IF \E s \in a_set: s.key = e /\ s.t = t THEN 
                          CHOOSE s \in a_set: s.key = e /\ s.t = t
                          ELSE [key |-> e, t |-> t, v_inn |-> 0, v_acq |-> 0, change |-> 0]
    _Get(set, e) == {struct \in set: struct.key = e}
    
    \* prepare phases
    _Add(a_set, r_set, self, num, e, x) == 
        IF ~_Lookup(a_set, r_set, e) THEN
            [key |-> e, val |-> x, t |-> <<self, num>>]
        ELSE [key |-> "null"]
    
    _Increase(a_set, r_set, e, i) == 
        IF _Lookup(a_set, r_set, e) THEN 
            [key |-> e, val |-> i, O |-> _Observe_T(a_set, r_set, e)]
        ELSE [key |-> "null"]
    
    _Remove(a_set, r_set, e) == 
        IF _Lookup(a_set, r_set, e) THEN 
            [key |-> e, O |-> _Observe_T(a_set, r_set, e)]
        ELSE [key |-> "null"] 
end define;

\* send an operation to all
macro Broadcast(o, params) begin 
    ops := [j \in Procs |-> IF j = self THEN ops[j] 
                            ELSE ops[j] \union {[op |-> o, num |-> opcount, p |-> params]}];
end macro;

\* update an element in Set
macro set_update(set, old, new) begin
    \* set := {IF tempx = old THEN new ELSE tempx: tempx \in set};
    set := (set \ {old}) \union {new};
end macro;

\* effect of remove operation
macro Remove(e, O) begin
    r_set := r_set \union {[key |-> e, t |-> ob_t]: ob_t \in O};
end macro;

\* effect of add operation
macro Add(e, x, t) begin
    with e_record = _Element(a_set, e, t) do
        set_update(a_set, e_record, [key |-> e, t |-> t, v_inn |-> x, 
                                    v_acq |-> e_record.v_acq, change |-> e_record.change]);
    end with;
end macro;

\* effect of increase operation
macro Increase(e, i, O) begin
    with inc_set_old = {_Element(a_set, e, t): t \in O} do
        with inc_set_new = {[key |-> rcd.key, t |-> rcd.t, v_inn |-> rcd.v_inn, 
                             v_acq |-> rcd.v_acq + i, change |-> rcd.change + Abs(i)]:
                            rcd \in inc_set_old} do
            a_set := (a_set \ inc_set_old) \union inc_set_new;
        end with;
    end with;
end macro;

\* receive and process operations, one by one
macro Update() begin 
    if ops[self] /= {} then
        with msg \in ops[self] do
            if msg.op = "A" then
                Add(msg.p.key, msg.p.val, msg.p.t);
            elsif msg.op = "R" then
                Remove(msg.p.key, msg.p.O);
            elsif msg.op = "I" then
                Increase(msg.p.key, msg.p.val, msg.p.O);
            end if;
            \* op_exed := op_exed \union {msg.num};
            ops[self] := ops[self] \ {msg}; \* clear processed operation
        end with;
    end if;
end macro;

process Set \in Procs
variables
    a_set = {}; \* local set of pairs --
                \* [key |-> "", t |-> [], v_inn |-> "", v_acq |-> "", change |-> ""] 
    r_set = {}; \* local set of pairs [key |-> "", t |-> []]
    num = 0;
    \* op_exed = {}; \* executed operations
begin Main:
    while TRUE do
        either
            if opcount < MaxOps then
                opcount := opcount + 1;
                either \* Add
                    with e \in Elmts, v \in Values, addp = _Add(a_set, r_set, self, num, e, v) do \* select a random element-value to add
                        if addp.key /= "null" then
                            Broadcast("A", addp);
                            num := num + 1;
                            Add(e, v, addp.t);
                            \* op_exed := op_exed \union {opcount};
                        end if;
                    end with;
                or \* Remove
                    with e \in Elmts, rmvp = _Remove(a_set, r_set, e) do \* select a random element to remove
                        if rmvp.key /= "null" then
                            Broadcast("R", rmvp);
                            Remove(e, rmvp.O);
                            \* op_exed := op_exed \union {opcount};
                        end if;
                    end with;
                or \* Increase
                    with e \in Elmts, i \in Increments, incp = _Increase(a_set, r_set, e, i) do \* select a random element-increment to inc
                        if incp.key /= "null" then
                            Broadcast("I", incp);
                            Increase(e, i, incp.O);
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
\* BEGIN TRANSLATION (chksum(pcal) = "e453ad5e" /\ chksum(tla) = "6ddd0792")
VARIABLES ops, opcount

(* define statement *)
Abs(x) == IF x < 0 THEN -x ELSE x

_T_Set(set, e) == {s.t: s \in {s \in set: s.key = e}}
_Observe_T(a_set, r_set, e) == _T_Set(a_set, e) \ _T_Set(r_set, e)
_Lookup(a_set, r_set, e) == _Observe_T(a_set, r_set, e) /= {}
_Element(a_set, e, t) == IF \E s \in a_set: s.key = e /\ s.t = t THEN
                      CHOOSE s \in a_set: s.key = e /\ s.t = t
                      ELSE [key |-> e, t |-> t, v_inn |-> 0, v_acq |-> 0, change |-> 0]
_Get(set, e) == {struct \in set: struct.key = e}


_Add(a_set, r_set, self, num, e, x) ==
    IF ~_Lookup(a_set, r_set, e) THEN
        [key |-> e, val |-> x, t |-> <<self, num>>]
    ELSE [key |-> "null"]

_Increase(a_set, r_set, e, i) ==
    IF _Lookup(a_set, r_set, e) THEN
        [key |-> e, val |-> i, O |-> _Observe_T(a_set, r_set, e)]
    ELSE [key |-> "null"]

_Remove(a_set, r_set, e) ==
    IF _Lookup(a_set, r_set, e) THEN
        [key |-> e, O |-> _Observe_T(a_set, r_set, e)]
    ELSE [key |-> "null"]

VARIABLES a_set, r_set, num

vars == << ops, opcount, a_set, r_set, num >>

ProcSet == (Procs)

Init == (* Global variables *)
        /\ ops = [j \in Procs |-> {}]
        /\ opcount = 0
        (* Process Set *)
        /\ a_set = [self \in Procs |-> {}]
        /\ r_set = [self \in Procs |-> {}]
        /\ num = [self \in Procs |-> 0]

Set(self) == \/ /\ IF opcount < MaxOps
                      THEN /\ opcount' = opcount + 1
                           /\ \/ /\ \E e \in Elmts:
                                      \E v \in Values:
                                        LET addp == _Add(a_set[self], r_set[self], self, num[self], e, v) IN
                                          IF addp.key /= "null"
                                             THEN /\ ops' = [j \in Procs |-> IF j = self THEN ops[j]
                                                                             ELSE ops[j] \union {[op |-> "A", num |-> opcount', p |-> addp]}]
                                                  /\ num' = [num EXCEPT ![self] = num[self] + 1]
                                                  /\ LET e_record == _Element(a_set[self], e, (addp.t)) IN
                                                       a_set' = [a_set EXCEPT ![self] = (a_set[self] \ {e_record}) \union {([key |-> e, t |-> (addp.t), v_inn |-> v,
                                                                                                                            v_acq |-> e_record.v_acq, change |-> e_record.change])}]
                                             ELSE /\ TRUE
                                                  /\ UNCHANGED << ops, 
                                                                  a_set, 
                                                                  num >>
                                 /\ r_set' = r_set
                              \/ /\ \E e \in Elmts:
                                      LET rmvp == _Remove(a_set[self], r_set[self], e) IN
                                        IF rmvp.key /= "null"
                                           THEN /\ ops' = [j \in Procs |-> IF j = self THEN ops[j]
                                                                           ELSE ops[j] \union {[op |-> "R", num |-> opcount', p |-> rmvp]}]
                                                /\ r_set' = [r_set EXCEPT ![self] = r_set[self] \union {[key |-> e, t |-> ob_t]: ob_t \in (rmvp.O)}]
                                           ELSE /\ TRUE
                                                /\ UNCHANGED << ops, r_set >>
                                 /\ UNCHANGED <<a_set, num>>
                              \/ /\ \E e \in Elmts:
                                      \E i \in Increments:
                                        LET incp == _Increase(a_set[self], r_set[self], e, i) IN
                                          IF incp.key /= "null"
                                             THEN /\ ops' = [j \in Procs |-> IF j = self THEN ops[j]
                                                                             ELSE ops[j] \union {[op |-> "I", num |-> opcount', p |-> incp]}]
                                                  /\ LET inc_set_old == {_Element(a_set[self], e, t): t \in (incp.O)} IN
                                                       LET inc_set_new == {[key |-> rcd.key, t |-> rcd.t, v_inn |-> rcd.v_inn,
                                                                            v_acq |-> rcd.v_acq + i, change |-> rcd.change + Abs(i)]:
                                                                           rcd \in inc_set_old} IN
                                                         a_set' = [a_set EXCEPT ![self] = (a_set[self] \ inc_set_old) \union inc_set_new]
                                             ELSE /\ TRUE
                                                  /\ UNCHANGED << ops, 
                                                                  a_set >>
                                 /\ UNCHANGED <<r_set, num>>
                      ELSE /\ TRUE
                           /\ UNCHANGED << ops, opcount, a_set, r_set, num >>
             \/ /\ IF ops[self] /= {}
                      THEN /\ \E msg \in ops[self]:
                                /\ IF msg.op = "A"
                                      THEN /\ LET e_record == _Element(a_set[self], (msg.p.key), (msg.p.t)) IN
                                                a_set' = [a_set EXCEPT ![self] = (a_set[self] \ {e_record}) \union {([key |-> (msg.p.key), t |-> (msg.p.t), v_inn |-> (msg.p.val),
                                                                                                                     v_acq |-> e_record.v_acq, change |-> e_record.change])}]
                                           /\ r_set' = r_set
                                      ELSE /\ IF msg.op = "R"
                                                 THEN /\ r_set' = [r_set EXCEPT ![self] = r_set[self] \union {[key |-> (msg.p.key), t |-> ob_t]: ob_t \in (msg.p.O)}]
                                                      /\ a_set' = a_set
                                                 ELSE /\ IF msg.op = "I"
                                                            THEN /\ LET inc_set_old == {_Element(a_set[self], (msg.p.key), t): t \in (msg.p.O)} IN
                                                                      LET inc_set_new == {[key |-> rcd.key, t |-> rcd.t, v_inn |-> rcd.v_inn,
                                                                                           v_acq |-> rcd.v_acq + (msg.p.val), change |-> rcd.change + Abs((msg.p.val))]:
                                                                                          rcd \in inc_set_old} IN
                                                                        a_set' = [a_set EXCEPT ![self] = (a_set[self] \ inc_set_old) \union inc_set_new]
                                                            ELSE /\ TRUE
                                                                 /\ a_set' = a_set
                                                      /\ r_set' = r_set
                                /\ ops' = [ops EXCEPT ![self] = ops[self] \ {msg}]
                      ELSE /\ TRUE
                           /\ UNCHANGED << ops, a_set, r_set >>
                /\ UNCHANGED <<opcount, num>>

Next == (\E self \in Procs: Set(self))

Spec == Init /\ [][Next]_vars

\* END TRANSLATION 


\* ops[p1] = ops[p2] => op_exed[p1] = op_exed[p2]
SEC == \A p1, p2 \in Procs: (p1 /= p2 /\ ops[p1] = ops[p2]) => 
                            (a_set[p1] = a_set[p2] /\ r_set[p1] = r_set[p2])

================================================================================
