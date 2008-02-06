
%module coot

%{

/* #ifdef USE_PYTHON */
/* #define SWIG_init SWIG_python_init */
/* #endif */

/* #ifdef USE_GUILE */
/* #define SWIG_init SWIG_guile_init */
/* #endif */

#include "globjects.h"  //includes gtk/gtk.h
#include "coot-coord-utils.hh"
#include "c-interface.h"
#include "cc-interface.hh"
#include "c-interface-database.hh"
#include "c-interface-python.hh"
%}


#include "globjects.h"  //includes gtk/gtk.h
#include "coot-coord-utils.hh"
%include "c-interface.h"
%include "cc-interface.hh"
%include "c-interface-database.hh"
%include "c-interface-python.hh"

#ifdef USE_PYTHON
extern PyObject *refmac_parameters_py(int imol);
extern PyObject *save_state_file_name_py();
extern int set_atom_attributes_py(PyObject *attribute_expression_list);
extern PyObject *merge_molecules_py(PyObject *add_molecules, int imol);
extern PyObject *change_chain_id_with_result_py(int imol, const char *from_chain_id, const char *to_chain_id, short int use_res_range_flag, int from_resno, int to_resno);
extern PyObject *chain_id_for_shelxl_residue_number_py(int imol, int resno);
extern PyObject *view_name_py(int view_number);
extern PyObject *view_description_py(int view_number);
extern void go_to_view_py(PyObject *view);
extern PyObject *movie_file_name_prefix_py();
extern PyObject *execute_ligand_search_py();  
extern PyObject *overlap_ligands_py(int imol_ligand, int imol_ref, const char *chain_id_ref, int resno_ref);
extern PyObject *analyse_ligand_differences_py(int imol_ligand, int imol_ref, const char *chain_id_ref, int resno_ref);
extern PyObject *cis_peptides_py(int imol);
extern PyObject *rotamer_graphs_py(int imol);
extern PyObject *apply_lsq_matches_py(int imol_reference, int imol_moving);
extern PyObject *ncs_chain_differences_py(int imol, const char *master_chain_id);
extern PyObject *ncs_chains_ids_py(int imol);
extern PyObject *sequence_info_py(int imol);
extern PyObject *drag_intermediate_atom_py(PyObject *atom_spec, PyObject *position);
extern PyObject *mark_atom_as_fixed_py(int imol, PyObject *atom_spec, int state);
extern PyObject *get_symmetry_py(int imol);
extern PyObject *map_colour_components_py(int imol);
extern PyObject *dictionaries_read_py();
extern PyObject *monomer_restraints_py(const char *monomer_type);
extern void set_monomer_restraints_py(const char *monomer_type, PyObject *restraints);
extern PyObject *safe_python_command_with_return(const std::string &python_cmd);
extern PyObject *py_residue(const coot::residue_spec_t &res);
extern PyObject *residue_info_py(int imol, const char* chain_id, int resno, const char *ins_code);
extern PyObject *residue_name_py(int imol, const char* chain_id, int resno, const char *ins_code);
extern int clear_and_update_molecule_py(int molecule_number, PyObject *molecule_expression);
extern int add_molecule_py(PyObject *molecule_expression, const char *name);
extern PyObject *active_residue_py();
extern PyObject *closest_atom_py(int imol);
extern void spin_search_py(int imol_map, int imol, const char *chain_id, int resno, const char *ins_code, PyObject *direction_atoms_list, PyObject *moving_atoms_list);
extern int cootaneer_py(int imol_map, int imol_model, PyObject *atom_in_fragment_atom_spec);
extern PyObject *generic_string_vector_to_list_internal_py(const std::vector<std::string>&v);
extern PyObject *generic_int_vector_to_list_internal_py(const std::vector<int> &v);
extern std::vector<std::string> generic_list_to_string_vector_internal_py(PyObject *l);
extern PyObject *rtop_to_python(const clipper::RTop_orth &rtop);
extern coot::atom_spec_t atom_spec_from_python_expression(PyObject *expr);
extern PyObject *display_python(PyObject *o);
extern std::pair<bool, coot::atom_spec_t> make_atom_spec_py(PyObject *spec);
#endif // USE_PYTHON
