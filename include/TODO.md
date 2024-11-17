# hybrid_move_structure.hpp

- [x] the C array currently used is the wrong one. build a new C array called
      C_H and keep the old one as it is for now

- [ ] c != last_c is the only thing we need to check, as the first character
      is always the start of a run

- [ ] Repetitive code, line 110 and line 120 due to the above issue
