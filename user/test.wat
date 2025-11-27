(module
  ;; Simple WASM test module
  ;; Exports a function "add" that adds two i32 numbers
  
  (func $add (export "add") (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    i32.add
  )
  
  (func $factorial (export "factorial") (param $n i32) (result i32)
    (local $result i32)
    (local $i i32)
    
    ;; Initialize result to 1
    i32.const 1
    local.set $result
    
    ;; Initialize i to 2
    i32.const 2
    local.set $i
    
    ;; Loop from 2 to n
    (block $break
      (loop $continue
        ;; Check if i > n
        local.get $i
        local.get $n
        i32.gt_s
        br_if $break
        
        ;; result *= i
        local.get $result
        local.get $i
        i32.mul
        local.set $result
        
        ;; i++
        local.get $i
        i32.const 1
        i32.add
        local.set $i
        
        br $continue
      )
    )
    
    local.get $result
  )
  
  (func $get_magic_number (export "get_magic_number") (result i32)
    i32.const 42
  )
)
