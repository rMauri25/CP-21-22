-module(break_md5).
-define(PASS_LEN, 6).
-define(UPDATE_BAR_GAP, 50000).
-define(BAR_SIZE, 40).
-define(THREADS, 26).

-export([break_md5s/1, break_md5/1, pass_to_num/1, num_to_pass/1]).
-export([progress_loop/3]).
-export([prueba/5]).

%T1= erlang:monotomic_time(microseconds),


% Base ^ Exp

% Base ^ Exp
pow_aux(_Base, Pow, 0) ->
    Pow;
pow_aux(Base, Pow, Exp) when Exp rem 2 == 0 ->
    pow_aux(Base*Base, Pow, Exp div 2);
pow_aux(Base, Pow, Exp) ->
    pow_aux(Base, Base * Pow, Exp - 1).

pow(Base, Exp) -> pow_aux(Base, 1, Exp).

%% Number to password and back conversion

num_to_pass_aux(_N, 0, Pass) -> Pass;
num_to_pass_aux(N, Digit, Pass) ->
    num_to_pass_aux(N div 26, Digit - 1, [$a + N rem 26 | Pass]).

num_to_pass(N) -> num_to_pass_aux(N, ?PASS_LEN, []).

pass_to_num(Pass) ->
    lists:foldl(fun (C, Num) -> Num * 26 + C - $a end, 0, Pass).

%% Hex string to Number

hex_char_to_int(N) ->
    if (N >= $0) and (N =< $9) -> N - $0;
       (N >= $a) and (N =< $f) -> N - $a + 10;
       (N >= $A) and (N =< $F) -> N - $A + 10;
       true                    -> throw({not_hex, [N]})
    end.

int_to_hex_char(N) ->
    if (N >= 0)  and (N < 10) -> $0 + N;
       (N >= 10) and (N < 16) -> $A + (N - 10);
       true                   -> throw({out_of_range, N})
    end.

hex_string_to_num(Hex_Str) ->
    lists:foldl(fun(Hex, Num) -> Num*16 + hex_char_to_int(Hex) end, 0, Hex_Str).

num_to_hex_string_aux(0, Str) -> Str;
num_to_hex_string_aux(N, Str) ->
    num_to_hex_string_aux(N div 16,
                          [int_to_hex_char(N rem 16) | Str]).

num_to_hex_string(0) -> "0";
num_to_hex_string(N) -> num_to_hex_string_aux(N, []).





%% Progress bar runs in its own process

progress_loop(N, Bound,T1) ->
    T2 = erlang:monotonic_time(microsecond),
    receive
        stop -> ok;
        {progress_report, Checked} ->
            
            N2 = N + Checked,
            Full_N = N2 * ?BAR_SIZE div Bound,
            Full = lists:duplicate(Full_N, $=),
            Empty = lists:duplicate(?BAR_SIZE - Full_N, $-),
            io:format("\r[~s~s] ~.2f% ~.2f b/s", [Full, Empty, N2/Bound*100, (N2/(T2 - T1)*1000000)]),
            progress_loop(N2, Bound,T1)
    end.

%% break_md5t/3 iterates checking the possible passwords




break_md5t([], _, _, _, _, Pid_principal) -> % No more hashes to find
    Pid_principal ! {exited, ok}, % Avisa al hilo principal
    exit(normal), % Finaliza la ejecucion del proceso
    ok;
break_md5t(Hashes,  Bound, Bound, _, _, Pid_principal) -> % Checked every possible password
    Pid_principal ! {not_found, Hashes}, %Avisa al hilo principal y le manda los hashes sin descifrar
    exit(normal),  % Finaliza la ejecicion del proceso
    ok;  
break_md5t(Hashes, N, Bound, Progress_Pid, Pid_list, Pid_principal) ->
    receive  
        {hash_found, New_Hashes} -> break_md5t(New_Hashes, N, Bound, Progress_Pid, Pid_list, Pid_principal) %Si alguien encontro una contraseña se actualiza la lista de hashes
    after
        0 -> true %Si no hay mensajes se sigue con la ejecucion del codigo
    end,
    

    
      if N rem ?UPDATE_BAR_GAP == 0 ->
            Progress_Pid ! {progress_report, ?UPDATE_BAR_GAP};
       true ->
            ok
    end,
    
    
    Pass = num_to_pass(N),
    Hash = crypto:hash(md5, Pass),
    Num_Hash = binary:decode_unsigned(Hash),
    case lists:member(Num_Hash, Hashes) of
        true ->
            if length(Hashes) == 1 ->
                    Progress_Pid ! stop;
                true ->
                    ok
            end,
            io:format("\e[2K\r~.16B: ~s ~p ~n", [Num_Hash, Pass, self()]), % Imprime la contraseña descifrada
            lists:foreach((fun(I) -> I ! {hash_found, lists:delete(Num_Hash, Hashes)} end), Pid_list), %Avisa al resto de procesos mandandoles la lista sin el hash descifrado
            break_md5t(Hashes, N+1, Bound, Progress_Pid, Pid_list, Pid_principal); 
        false ->
            break_md5t(Hashes, N+1, Bound, Progress_Pid, Pid_list, Pid_principal)
    end.

%% prueba/5 espera a recibir la lista con los pids de los procesos que descodifican
prueba(Hashes, N, Bound, Progress_Pid, Pid_principal) ->
    receive
        {pid_list, Pid_list} -> break_md5t(Hashes, N, Bound, Progress_Pid, Pid_list, Pid_principal)
    end.

%% Funcion que crea los hilos y cuando todos estan creados les manda la lista con los pids
%% de los hilos que estan descodificando
create_loop(0, _, _, _, Pid_list) ->
    Fun = fun(N) -> N ! {pid_list, Pid_list} end,
    lists:foreach(Fun, Pid_list);

create_loop(N, Num_Hashes, Bound_t, Progress_Pid, Pid_list) ->
    Pid = spawn(?MODULE, prueba, [Num_Hashes, Bound_t * (N-1), Bound_t * N, Progress_Pid, self()]),
    link(Pid),
    create_loop(N-1, Num_Hashes, Bound_t, Progress_Pid, [Pid|Pid_list]).


%% Funcion que espera a que todos los hilos hayan terminado
wait(Num_threads, Num_threads, Not_found, Empty) -> 
    if(Empty == true) ->
        ok;
    true -> {not_found, lists:usort(Not_found)}
    end;

wait(Num_threads, N, _, Empty) ->
    receive 
        {exited, ok} -> 
            wait(Num_threads, N+1, [], true);
        {not_found, Hashes} -> 
            wait(Num_threads, N+1, Hashes, (false or Empty))
    end.


%% Break a list of hashes

break_md5s(Hashes) ->
    T1 = erlang:monotonic_time(microsecond),
    Bound = pow(26, ?PASS_LEN),
    Progress_Pid = spawn(?MODULE, progress_loop, [0, Bound,T1]),
    link(Progress_Pid),
    Num_Hashes = lists:map(fun hex_string_to_num/1, Hashes),
    %dividimos Bound entre los hilos
    Bound_t = Bound div ?THREADS,
    %bucle que crea los hilos
    create_loop(?THREADS, Num_Hashes, Bound_t, Progress_Pid, []),
    %Esperamos a que finalicen todos los hilos
    Res = wait(?THREADS, 0, [], false),
    c:flush(),
    io:format("~n"),
    Res.

%% Break a single hash

break_md5(Hash) -> break_md5s([Hash]).
