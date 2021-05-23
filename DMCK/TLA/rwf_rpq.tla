-------------------------------- MODULE rwf_rpq --------------------------------
EXTENDS Integers, Sequences, TLC
CONSTANTS N, MaxOps, Elmts, Values, Increments

Dft_Values == {10, 20}
Dft_Increments == {-3, 4}
Dft_Elmts == {"e"}

Procs == 1..N 

(*--algorithm rwf_rpq

variables
    ops = [j \in Procs |-> {}]; \* to broadcast operations
    opcount = 0;
    history = <<>>; \* history variable

define \* prepare phases
    Max(a, b) == IF a <= b THEN b ELSE a
    Max_RH(a, b) == [j \in Procs |-> IF a[j] <= b[j] THEN b[j] ELSE a[j]]
    _Lookup(e_set, e) == \E s \in e_set: s.key = e
    _ELEMENT(e_set, e) == IF \E s \in e_set: s.key = e THEN 
                          CHOOSE s \in e_set: s.key = e
                          ELSE [key |-> e, p_ini |-> -1, v_inn |-> 0, v_acq |-> 0]
    _RH(t_set, e) == IF \E s \in t_set: s.key = e THEN
                        CHOOSE t \in {s.t: s \in t_set}: TRUE
                     ELSE [j \in Procs |-> 0]
    _Get(set, e) == {struct \in set: struct.key = e}
    
    _Add(e_set, t_set, self, e, x) == 
        IF ~_Lookup(e_set, e) THEN
            [key |-> e, val |-> x, p_ini |-> self, rh |-> _RH(t_set, e)]
        ELSE [key |-> "null"]
    
    _Increase(e_set, t_set, e, i) == 
        IF _Lookup(e_set, e) THEN [key |-> e, val |-> i, rh |-> _RH(t_set, e)]
        ELSE [key |-> "null"]
    
    _Remove(e_set, t_set, self, e) == 
        IF _Lookup(e_set, e) THEN 
            [key |-> e, rh |-> [j \in Procs |-> IF j = self THEN _RH(t_set, e)[j] + 1 
                                                           ELSE _RH(t_set, e)[j]]]
        ELSE [key |-> "null", rh |-> [j \in Procs |-> -1]] 
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
macro Remove(e, rhp) begin
    assert rhp /= [j \in Procs |-> -1];
    with local_rh = _RH(t_set, e) do
        if \E j \in Procs: local_rh[j] < rhp[j] then
            set_update(t_set, [key |-> e, t |-> local_rh],
                       [key |-> e, t |-> Max_RH(rhp, local_rh)]);
            e_set := e_set \ _Get(e_set, e);
        end if;
    end with;
end macro;

\* effect of add operation
macro Add(e, x, p_ini, rh) begin
    with local_rh = _RH(t_set, e) do
        if \E j \in Procs: local_rh[j] < rh[j] then
            set_update(t_set, [key |-> e, t |-> local_rh],
                       [key |-> e, t |-> [j \in Procs |-> Max(rh[j], local_rh[j])]]);
            with ele = _ELEMENT(e_set, e) do
                if rh = _RH(t_set, e) then
                    set_update(e_set, ele, [key |-> e, p_ini |-> p_ini, v_inn |-> x, v_acq |-> 0]);
                else e_set := e_set \ _Get(e_set, e);
                end if;
            end with;
        else
            with ele = _ELEMENT(e_set, e) do
                if rh = _RH(t_set, e) /\ p_ini > ele.p_ini then
                    set_update(e_set, ele, [key |-> e, p_ini |-> p_ini, v_inn |-> x, v_acq |-> ele.v_acq]);
                end if;
            end with;
        end if;
    end with;
end macro;

\* effect of increase operation
macro Increase(e, i, rh) begin
    with local_rh = _RH(t_set, e) do
        if \E j \in Procs: local_rh[j] < rh[j] then
            set_update(t_set, [key |-> e, t |-> local_rh],
                       [key |-> e, t |-> [j \in Procs |-> Max(rh[j], local_rh[j])]]);
            with ele = _ELEMENT(e_set, e) do
                if rh = _RH(t_set, e) then
                    set_update(e_set, ele, [key |-> e, p_ini |-> -1, v_inn |-> 0, v_acq |-> 0 + i]);
                else e_set := e_set \ _Get(e_set, e);
                end if;
            end with;
        else
            with ele = _ELEMENT(e_set, e) do
                if rh = _RH(t_set, e) then
                    set_update(e_set, ele, [key |-> e, p_ini |-> ele.p_ini, v_inn |-> ele.v_inn, v_acq |-> ele.v_acq + i]);
                end if;
            end with;
        end if;
    end with;
end macro;

\* receive and process operations, one by one
macro Update() begin 
    if ops[self] /= {} then
        with msg \in ops[self] do
            if msg.op = "A" then
                Add(msg.p.key, msg.p.val, msg.p.p_ini, msg.p.rh);
            elsif msg.op = "R" then
                Remove(msg.p.key, msg.p.rh);
            elsif msg.op = "I" then
                Increase(msg.p.key, msg.p.val, msg.p.rh);
            end if;
            history := Append(history, <<msg.num, self, "effect">>);
            op_exed := op_exed \union {msg.num};
            ops[self] := ops[self] \ {msg}; \* clear processed operation
        end with;
    end if;
end macro;

process Set \in Procs
variables
    e_set = {}; \* local set of pairs [key |-> "", p_ini |-> "", v_inn |-> "", v_acq |-> ""]
    t_set = {}; \* local set of pairs [key |-> "", t |-> []]
    op_exed = {}; \* executed operations
begin Main:
    while TRUE do
        either
            if opcount < MaxOps then
                opcount := opcount + 1;
                either \* Add
                    with e \in Elmts, v \in Values, addp = _Add(e_set, t_set, self, e, v) do \* select a random element-value to add
                        history := Append(history, <<opcount, self, "Add", e, v>>);
                        if addp.key /= "null" then
                            Broadcast("A", addp);
                            Add(e, v, addp.p_ini, addp.rh);
                            op_exed := op_exed \union {opcount};
                        end if;
                    end with;
                or \* Remove
                    with e \in Elmts, rmvp = _Remove(e_set, t_set, self, e) do \* select a random element to remove
                        history := Append(history, <<opcount, self, "Rmv", e>>);
                        if rmvp.key /= "null" then
                            Broadcast("R", rmvp);
                            Remove(e, rmvp.rh);
                            op_exed := op_exed \union {opcount};
                        end if;
                    end with;
                or \* Increase
                    with e \in Elmts, i \in Increments, incp = _Increase(e_set, t_set, e, i) do \* select a random element-increment to inc
                        history := Append(history, <<opcount, self, "Inc", e, i>>);
                        if incp.key /= "null" then
                            Broadcast("I", incp);
                            Increase(e, i, incp.rh);
                            op_exed := op_exed \union {opcount};
                        end if;
                    end with;
                end either;
            end if;
        or Update();
        end either;
    end while;
end process;

end algorithm;*)
\* BEGIN TRANSLATION (chksum(pcal) = "a96a5402" /\ chksum(tla) = "4e596614")
VARIABLES ops, opcount, history

(* define statement *)
Max(a, b) == IF a <= b THEN b ELSE a
Max_RH(a, b) == [j \in Procs |-> IF a[j] <= b[j] THEN b[j] ELSE a[j]]
_Lookup(e_set, e) == \E s \in e_set: s.key = e
_ELEMENT(e_set, e) == IF \E s \in e_set: s.key = e THEN
                      CHOOSE s \in e_set: s.key = e
                      ELSE [key |-> e, p_ini |-> -1, v_inn |-> 0, v_acq |-> 0]
_RH(t_set, e) == IF \E s \in t_set: s.key = e THEN
                    CHOOSE t \in {s.t: s \in t_set}: TRUE
                 ELSE [j \in Procs |-> 0]
_Get(set, e) == {struct \in set: struct.key = e}

_Add(e_set, t_set, self, e, x) ==
    IF ~_Lookup(e_set, e) THEN
        [key |-> e, val |-> x, p_ini |-> self, rh |-> _RH(t_set, e)]
    ELSE [key |-> "null"]

_Increase(e_set, t_set, e, i) ==
    IF _Lookup(e_set, e) THEN [key |-> e, val |-> i, rh |-> _RH(t_set, e)]
    ELSE [key |-> "null"]

_Remove(e_set, t_set, self, e) ==
    IF _Lookup(e_set, e) THEN
        [key |-> e, rh |-> [j \in Procs |-> IF j = self THEN _RH(t_set, e)[j] + 1
                                                       ELSE _RH(t_set, e)[j]]]
    ELSE [key |-> "null", rh |-> [j \in Procs |-> -1]]

VARIABLES e_set, t_set, op_exed

vars == << ops, opcount, history, e_set, t_set, op_exed >>

ProcSet == (Procs)

Init == (* Global variables *)
        /\ ops = [j \in Procs |-> {}]
        /\ opcount = 0
        /\ history = <<>>
        (* Process Set *)
        /\ e_set = [self \in Procs |-> {}]
        /\ t_set = [self \in Procs |-> {}]
        /\ op_exed = [self \in Procs |-> {}]

Set(self) == \/ /\ IF opcount < MaxOps
                      THEN /\ opcount' = opcount + 1
                           /\ \/ /\ \E e \in Elmts:
                                      \E v \in Values:
                                        LET addp == _Add(e_set[self], t_set[self], self, e, v) IN
                                          /\ history' = Append(history, <<opcount', self, "Add", e, v>>)
                                          /\ IF addp.key /= "null"
                                                THEN /\ ops' = [j \in Procs |-> IF j = self THEN ops[j]
                                                                                ELSE ops[j] \union {[op |-> "A", num |-> opcount', p |-> addp]}]
                                                     /\ LET local_rh == _RH(t_set[self], e) IN
                                                          IF \E j \in Procs: local_rh[j] < (addp.rh)[j]
                                                             THEN /\ t_set' = [t_set EXCEPT ![self] = (t_set[self] \ {([key |-> e, t |-> local_rh])}) \union {([key |-> e, t |-> [j \in Procs |-> Max((addp.rh)[j], local_rh[j])]])}]
                                                                  /\ LET ele == _ELEMENT(e_set[self], e) IN
                                                                       IF (addp.rh) = _RH(t_set'[self], e)
                                                                          THEN /\ e_set' = [e_set EXCEPT ![self] = (e_set[self] \ {ele}) \union {([key |-> e, p_ini |-> (addp.p_ini), v_inn |-> v, v_acq |-> 0])}]
                                                                          ELSE /\ e_set' = [e_set EXCEPT ![self] = e_set[self] \ _Get(e_set[self], e)]
                                                             ELSE /\ LET ele == _ELEMENT(e_set[self], e) IN
                                                                       IF (addp.rh) = _RH(t_set[self], e) /\ (addp.p_ini) > ele.p_ini
                                                                          THEN /\ e_set' = [e_set EXCEPT ![self] = (e_set[self] \ {ele}) \union {([key |-> e, p_ini |-> (addp.p_ini), v_inn |-> v, v_acq |-> ele.v_acq])}]
                                                                          ELSE /\ TRUE
                                                                               /\ e_set' = e_set
                                                                  /\ t_set' = t_set
                                                     /\ op_exed' = [op_exed EXCEPT ![self] = op_exed[self] \union {opcount'}]
                                                ELSE /\ TRUE
                                                     /\ UNCHANGED << ops, 
                                                                     e_set, 
                                                                     t_set, 
                                                                     op_exed >>
                              \/ /\ \E e \in Elmts:
                                      LET rmvp == _Remove(e_set[self], t_set[self], self, e) IN
                                        /\ history' = Append(history, <<opcount', self, "Rmv", e>>)
                                        /\ IF rmvp.key /= "null"
                                              THEN /\ ops' = [j \in Procs |-> IF j = self THEN ops[j]
                                                                              ELSE ops[j] \union {[op |-> "R", num |-> opcount', p |-> rmvp]}]
                                                   /\ Assert((rmvp.rh) /= [j \in Procs |-> -1], 
                                                             "Failure of assertion at line 60, column 5 of macro called at line 156, column 29.")
                                                   /\ LET local_rh == _RH(t_set[self], e) IN
                                                        IF \E j \in Procs: local_rh[j] < (rmvp.rh)[j]
                                                           THEN /\ t_set' = [t_set EXCEPT ![self] = (t_set[self] \ {([key |-> e, t |-> local_rh])}) \union {([key |-> e, t |-> Max_RH((rmvp.rh), local_rh)])}]
                                                                /\ e_set' = [e_set EXCEPT ![self] = e_set[self] \ _Get(e_set[self], e)]
                                                           ELSE /\ TRUE
                                                                /\ UNCHANGED << e_set, 
                                                                                t_set >>
                                                   /\ op_exed' = [op_exed EXCEPT ![self] = op_exed[self] \union {opcount'}]
                                              ELSE /\ TRUE
                                                   /\ UNCHANGED << ops, 
                                                                   e_set, 
                                                                   t_set, 
                                                                   op_exed >>
                              \/ /\ \E e \in Elmts:
                                      \E i \in Increments:
                                        LET incp == _Increase(e_set[self], t_set[self], e, i) IN
                                          /\ history' = Append(history, <<opcount', self, "Inc", e, i>>)
                                          /\ IF incp.key /= "null"
                                                THEN /\ ops' = [j \in Procs |-> IF j = self THEN ops[j]
                                                                                ELSE ops[j] \union {[op |-> "I", num |-> opcount', p |-> incp]}]
                                                     /\ LET local_rh == _RH(t_set[self], e) IN
                                                          IF \E j \in Procs: local_rh[j] < (incp.rh)[j]
                                                             THEN /\ t_set' = [t_set EXCEPT ![self] = (t_set[self] \ {([key |-> e, t |-> local_rh])}) \union {([key |-> e, t |-> [j \in Procs |-> Max((incp.rh)[j], local_rh[j])]])}]
                                                                  /\ LET ele == _ELEMENT(e_set[self], e) IN
                                                                       IF (incp.rh) = _RH(t_set'[self], e)
                                                                          THEN /\ e_set' = [e_set EXCEPT ![self] = (e_set[self] \ {ele}) \union {([key |-> e, p_ini |-> -1, v_inn |-> 0, v_acq |-> 0 + i])}]
                                                                          ELSE /\ e_set' = [e_set EXCEPT ![self] = e_set[self] \ _Get(e_set[self], e)]
                                                             ELSE /\ LET ele == _ELEMENT(e_set[self], e) IN
                                                                       IF (incp.rh) = _RH(t_set[self], e)
                                                                          THEN /\ e_set' = [e_set EXCEPT ![self] = (e_set[self] \ {ele}) \union {([key |-> e, p_ini |-> ele.p_ini, v_inn |-> ele.v_inn, v_acq |-> ele.v_acq + i])}]
                                                                          ELSE /\ TRUE
                                                                               /\ e_set' = e_set
                                                                  /\ t_set' = t_set
                                                     /\ op_exed' = [op_exed EXCEPT ![self] = op_exed[self] \union {opcount'}]
                                                ELSE /\ TRUE
                                                     /\ UNCHANGED << ops, 
                                                                     e_set, 
                                                                     t_set, 
                                                                     op_exed >>
                      ELSE /\ TRUE
                           /\ UNCHANGED << ops, opcount, history, e_set, 
                                           t_set, op_exed >>
             \/ /\ IF ops[self] /= {}
                      THEN /\ \E msg \in ops[self]:
                                /\ IF msg.op = "A"
                                      THEN /\ LET local_rh == _RH(t_set[self], (msg.p.key)) IN
                                                IF \E j \in Procs: local_rh[j] < (msg.p.rh)[j]
                                                   THEN /\ t_set' = [t_set EXCEPT ![self] = (t_set[self] \ {([key |-> (msg.p.key), t |-> local_rh])}) \union {([key |-> (msg.p.key), t |-> [j \in Procs |-> Max((msg.p.rh)[j], local_rh[j])]])}]
                                                        /\ LET ele == _ELEMENT(e_set[self], (msg.p.key)) IN
                                                             IF (msg.p.rh) = _RH(t_set'[self], (msg.p.key))
                                                                THEN /\ e_set' = [e_set EXCEPT ![self] = (e_set[self] \ {ele}) \union {([key |-> (msg.p.key), p_ini |-> (msg.p.p_ini), v_inn |-> (msg.p.val), v_acq |-> 0])}]
                                                                ELSE /\ e_set' = [e_set EXCEPT ![self] = e_set[self] \ _Get(e_set[self], (msg.p.key))]
                                                   ELSE /\ LET ele == _ELEMENT(e_set[self], (msg.p.key)) IN
                                                             IF (msg.p.rh) = _RH(t_set[self], (msg.p.key)) /\ (msg.p.p_ini) > ele.p_ini
                                                                THEN /\ e_set' = [e_set EXCEPT ![self] = (e_set[self] \ {ele}) \union {([key |-> (msg.p.key), p_ini |-> (msg.p.p_ini), v_inn |-> (msg.p.val), v_acq |-> ele.v_acq])}]
                                                                ELSE /\ TRUE
                                                                     /\ e_set' = e_set
                                                        /\ t_set' = t_set
                                      ELSE /\ IF msg.op = "R"
                                                 THEN /\ Assert((msg.p.rh) /= [j \in Procs |-> -1], 
                                                                "Failure of assertion at line 60, column 5 of macro called at line 171, column 12.")
                                                      /\ LET local_rh == _RH(t_set[self], (msg.p.key)) IN
                                                           IF \E j \in Procs: local_rh[j] < (msg.p.rh)[j]
                                                              THEN /\ t_set' = [t_set EXCEPT ![self] = (t_set[self] \ {([key |-> (msg.p.key), t |-> local_rh])}) \union {([key |-> (msg.p.key), t |-> Max_RH((msg.p.rh), local_rh)])}]
                                                                   /\ e_set' = [e_set EXCEPT ![self] = e_set[self] \ _Get(e_set[self], (msg.p.key))]
                                                              ELSE /\ TRUE
                                                                   /\ UNCHANGED << e_set, 
                                                                                   t_set >>
                                                 ELSE /\ IF msg.op = "I"
                                                            THEN /\ LET local_rh == _RH(t_set[self], (msg.p.key)) IN
                                                                      IF \E j \in Procs: local_rh[j] < (msg.p.rh)[j]
                                                                         THEN /\ t_set' = [t_set EXCEPT ![self] = (t_set[self] \ {([key |-> (msg.p.key), t |-> local_rh])}) \union {([key |-> (msg.p.key), t |-> [j \in Procs |-> Max((msg.p.rh)[j], local_rh[j])]])}]
                                                                              /\ LET ele == _ELEMENT(e_set[self], (msg.p.key)) IN
                                                                                   IF (msg.p.rh) = _RH(t_set'[self], (msg.p.key))
                                                                                      THEN /\ e_set' = [e_set EXCEPT ![self] = (e_set[self] \ {ele}) \union {([key |-> (msg.p.key), p_ini |-> -1, v_inn |-> 0, v_acq |-> 0 + (msg.p.val)])}]
                                                                                      ELSE /\ e_set' = [e_set EXCEPT ![self] = e_set[self] \ _Get(e_set[self], (msg.p.key))]
                                                                         ELSE /\ LET ele == _ELEMENT(e_set[self], (msg.p.key)) IN
                                                                                   IF (msg.p.rh) = _RH(t_set[self], (msg.p.key))
                                                                                      THEN /\ e_set' = [e_set EXCEPT ![self] = (e_set[self] \ {ele}) \union {([key |-> (msg.p.key), p_ini |-> ele.p_ini, v_inn |-> ele.v_inn, v_acq |-> ele.v_acq + (msg.p.val)])}]
                                                                                      ELSE /\ TRUE
                                                                                           /\ e_set' = e_set
                                                                              /\ t_set' = t_set
                                                            ELSE /\ TRUE
                                                                 /\ UNCHANGED << e_set, 
                                                                                 t_set >>
                                /\ history' = Append(history, <<msg.num, self, "effect">>)
                                /\ op_exed' = [op_exed EXCEPT ![self] = op_exed[self] \union {msg.num}]
                                /\ ops' = [ops EXCEPT ![self] = ops[self] \ {msg}]
                      ELSE /\ TRUE
                           /\ UNCHANGED << ops, history, e_set, t_set, 
                                           op_exed >>
                /\ UNCHANGED opcount

Next == (\E self \in Procs: Set(self))

Spec == Init /\ [][Next]_vars

\* END TRANSLATION 


================================================================================
