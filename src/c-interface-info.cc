/* src/c-interface.cc
 * 
 * Copyright 2002, 2003, 2004, 2005, 2006 The University of York
 * Author: Paul Emsley
 * Copyright 2007 by Paul Emsley
 * Copyright 2007 The University of Oxford
 * Author: Paul Emsley
 * Copyright 2007 by Bernhard Lohkamp
 * Copyright 2007 The University of York
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

#include <stdlib.h>
#include <iostream>
#include <fstream>


#define HAVE_CIF  // will become unnessary at some stage.

#include <sys/types.h> // for stating
#include <sys/stat.h>

#if !defined _MSC_VER
#include <unistd.h>
#else
#include <windows.h>
#define snprintf _snprintf
#endif
 
#include "globjects.h" //includes gtk/gtk.h

#include "callbacks.h"
#include "interface.h" // now that we are moving callback
		       // functionality to the file, we need this
		       // header since some of the callbacks call
		       // fuctions built by glade.

#include <vector>
#include <string>

#include "mmdb_manager.h"
#include "mmdb-extras.h"
#include "mmdb.h"
#include "mmdb-crystal.h"

#include "Cartesian.h"
#include "Bond_lines.h"

// #include "xmap-interface.h"
#include "graphics-info.h"

#include "atom-utils.h" // asc_to_graphics
// #include "db-main.h" not yet

#include "BuildCas.h"
#include "chi-angles.hh"

#include "trackball.h" // adding exportable rotate interface

#include "c-interface.h"
#include "coot-database.hh"

#ifdef USE_GUILE
#include <guile/gh.h>
#if (SCM_MAJOR_VERSION > 1) || (SCM_MINOR_VERSION > 7)
// no fix up needed 
#else    
#define scm_to_int gh_scm2int
#define scm_to_locale_string SCM_STRING_CHARS
#define scm_to_double  gh_scm2double
#define scm_is_true gh_scm2bool
#define scm_car SCM_CAR
#endif // SCM version
#endif // USE_GUILE

// Including python needs to come after graphics-info.h, because
// something in Python.h (2.4 - chihiro) is redefining FF1 (in
// ssm_superpose.h) to be 0x00004000 (Grrr).
//
#ifdef USE_PYTHON
#include "Python.h"
#if (PY_MINOR_VERSION > 4) 
// no fixup needed 
#else
#define Py_ssize_t int
#endif
#endif // USE_PYTHON

#include "cc-interface.hh"

/*  ------------------------------------------------------------------------ */
/*                         Molecule Functions       :                        */
/*  ------------------------------------------------------------------------ */
/* section Molecule Functions */
// return status, less than -9999 is for failure (eg. bad imol);
float molecule_centre_internal(int imol, int iaxis) {

   float fstat = -10000;

   if (is_valid_model_molecule(imol)) {
      if (iaxis >=0 && iaxis <=2) { 
	 coot::Cartesian c =
	    centre_of_molecule(graphics_info_t::molecules[imol].atom_sel);
	 if (iaxis == 0)
	    return c.x();
	 if (iaxis == 1)
	    return c.y();
	 if (iaxis == 2)
	    return c.z();
      }
   } else {
      std::cout << "WARNING: molecule " << imol
		<< " is not a valid model molecule number " << std::endl;
   }
   return fstat;
}

int is_shelx_molecule(int imol) {

   int r=0;
   if (is_valid_model_molecule(imol)) {
      r = graphics_info_t::molecules[imol].is_from_shelx_ins();
   }
   return r;

}


/*  ----------------------------------------------------------------------- */
/*               (Eleanor's) Residue info                                   */
/*  ----------------------------------------------------------------------- */
/* Similar to above, we need only one click though. */
void do_residue_info_dialog() {

   if (graphics_info_t::residue_info_edits->size() > 0) {

      std::string s =  "You have pending (un-Applied) residue edits\n";
      s += "Deal with them first.";
      GtkWidget *w = wrapped_nothing_bad_dialog(s);
      gtk_widget_show(w);
   } else { 
      std::cout << "Click on an atom..." << std::endl;
      graphics_info_t g;
      g.in_residue_info_define = 1;
      pick_cursor_maybe();
      graphics_info_t::pick_pending_flag = 1;
   }
} 

#ifdef USE_GUILE
SCM sequence_info(int imol) {

   SCM r = SCM_EOL;

   if (is_valid_model_molecule(imol)) { 
      std::vector<std::pair<std::string, std::string> > seq =
	 graphics_info_t::molecules[imol].sequence_info();
      
      if (seq.size() > 0) {
	 // unsigned int does't work here because then the termination
	 // condition never fails.
	 for (int iv=int(seq.size()-1); iv>=0; iv--) {
	    std::cout << "iv: " << iv << " seq.size: " << seq.size() << std::endl;
	    std::cout << "debug scming" << seq[iv].first.c_str()
		      << " and " << seq[iv].second.c_str() << std::endl;
	    SCM a = scm_makfrom0str(seq[iv].first.c_str());
	    SCM b = scm_makfrom0str(seq[iv].second.c_str());
	    SCM ls = scm_cons(a, b);
	    r = scm_cons(ls, r);
	 }
      }
   }
   return r;
} 
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *sequence_info_py(int imol) {

   PyObject *r;
   r = PyList_New(0);

   if (is_valid_model_molecule(imol)) { 
      
      std::vector<std::pair<std::string, std::string> > seq =
	 graphics_info_t::molecules[imol].sequence_info();


      if (seq.size() > 0) {
	 // unsigned int does't work here because then the termination
	 // condition never fails.
         r = PyList_New(seq.size());
	 for (int iv=int(seq.size()-1); iv>=0; iv--) {
	    std::cout << "iv: " << iv << " seq.size: " << seq.size() << std::endl;
	    std::cout << "debug pythoning " << seq[iv].first.c_str()
		      << " and " << seq[iv].second.c_str() << std::endl;
	    PyObject *a = PyString_FromString(seq[iv].first.c_str());
	    PyObject *b = PyString_FromString(seq[iv].second.c_str());
	    PyObject *ls = PyList_New(2);
            PyList_SetItem(ls, 0, a);
            PyList_SetItem(ls, 1, b);
            PyList_SetItem(r, iv, ls);
	 }
      }
   }
   return r;
} 
#endif // USE_PYTHON


// Called from a graphics-info-defines routine, would you believe? :)
//
// This should be a graphics_info_t function. 
//
// The reader is graphics_info_t::apply_residue_info_changes(GtkWidget *dialog);
// 
void output_residue_info_dialog(int atom_index, int imol) {

   if (graphics_info_t::residue_info_edits->size() > 0) {

      std::string s =  "You have pending (un-Applied) residue edits.\n";
      s += "Deal with them first.";
      GtkWidget *w = wrapped_nothing_bad_dialog(s);
      gtk_widget_show(w);

   } else { 

      if (imol <graphics_info_t::n_molecules()) {
	 if (graphics_info_t::molecules[imol].has_model()) {
	    if (atom_index < graphics_info_t::molecules[imol].atom_sel.n_selected_atoms) { 

	       graphics_info_t g;
	       output_residue_info_as_text(atom_index, imol);
	       PCAtom picked_atom = g.molecules[imol].atom_sel.atom_selection[atom_index];
   
	       PPCAtom atoms;
	       int n_atoms;

	       picked_atom->residue->GetAtomTable(atoms,n_atoms);
	       GtkWidget *widget = wrapped_create_residue_info_dialog();

	       CResidue *residue = picked_atom->residue; 
	       coot::residue_spec_t *res_spec_p =
		  new coot::residue_spec_t(residue->GetChainID(),
					   residue->GetSeqNum(),
					   residue->GetInsCode());
	       
	       // fill the master atom
	       GtkWidget *master_occ_entry =
		  lookup_widget(widget, "residue_info_master_atom_occ_entry"); 
	       GtkWidget *master_b_factor_entry =
		  lookup_widget(widget, "residue_info_master_atom_b_factor_entry");

	       gtk_signal_connect (GTK_OBJECT (master_occ_entry), "changed",
				   GTK_SIGNAL_FUNC (graphics_info_t::on_residue_info_master_atom_occ_changed),
				   NULL);
	       gtk_signal_connect (GTK_OBJECT (master_b_factor_entry),
				   "changed",
				   GTK_SIGNAL_FUNC (graphics_info_t::on_residue_info_master_atom_b_factor_changed),
				   NULL);


	       gtk_entry_set_text(GTK_ENTRY(master_occ_entry), "1.00");
	       gtk_entry_set_text(GTK_ENTRY(master_b_factor_entry),
				  graphics_info_t::float_to_string(graphics_info_t::default_new_atoms_b_factor).c_str());
					   
	       gtk_object_set_user_data(GTK_OBJECT(widget), res_spec_p);
	       g.fill_output_residue_info_widget(widget, imol, atoms, n_atoms);
	       gtk_widget_show(widget);
	       g.reset_residue_info_edits();

	       coot::chi_angles chi_angles(residue, 0);
	       std::vector<std::pair<int, float> > chis = chi_angles.get_chi_angles();
	       if (chis.size() > 0) {
		  GtkWidget *chi_angles_frame = lookup_widget(widget, "chi_angles_frame");
		  gtk_widget_show(chi_angles_frame);
		  for (unsigned int ich=0; ich<chis.size(); ich++) {

		     int ic = chis[ich].first;
		     std::string label_name = "residue_info_chi_";
		     label_name += coot::util::int_to_string(ic);
		     label_name += "_label";
		     GtkWidget *label = lookup_widget(widget, label_name.c_str());
		     if (label) {
			std::string text = "Chi ";
			text += coot::util::int_to_string(ic);
			text += ":  ";
			text += coot::util::float_to_string(chis[ich].second);
			text += " degrees";
			gtk_label_set_text(GTK_LABEL(label), text.c_str());
			gtk_widget_show(label);
		     } else {
			std::cout << "WARNING:: chi label not found " << label_name << std::endl;
		     } 
		  } 
	       }
	    }
	 }
      }
   }

   std::string cmd = "output-residue-info";
   std::vector<coot::command_arg_t> args;
   args.push_back(atom_index);
   args.push_back(imol);
   add_to_history_typed(cmd, args);
}

void residue_info_dialog(int imol, const char *chain_id, int resno, const char *ins_code) {

   if (is_valid_model_molecule(imol)) {
      int atom_index = -1;
      CResidue *res = graphics_info_t::molecules[imol].residue_from_external(resno, ins_code, chain_id);
      PPCAtom residue_atoms;
      int n_residue_atoms;
      res->GetAtomTable(residue_atoms, n_residue_atoms);
      if (n_residue_atoms > 0) {
	 CAtom *at = residue_atoms[0];
	 int handle = graphics_info_t::molecules[imol].atom_sel.UDDAtomIndexHandle;
	 int ierr = at->GetUDData(handle, atom_index);
	 if (ierr == UDDATA_Ok)
	    if (atom_index != -1) 
	       output_residue_info_dialog(atom_index, imol);
      }
   }
}


// 23 Oct 2003: Why is this so difficult?  Because we want to attach
// atom info (what springs to mind is a pointer to the atom) for each
// entry, so that when the text in the entry is changed, we know to
// modify the atom.
//
// The problem with that is that behind our backs, that atom could
// disappear (e.g close molecule or delete residue, mutate or
// whatever), we are left with a valid looking (i.e. non-NULL)
// pointer, but the memory to which is points is invalid -> crash when
// we try to reference it.
//
// How shall we get round this?  refcounting?
//
// Instead, let's make a trivial class that contains the information
// we need to do a SelectAtoms to find the pointer to the atom, that
// class shall be called select_atom_info, it shall contain the
// molecule number, the chain id, the residue number, the insertion
// code, the atom name, the atom altconf.
// 


void output_residue_info_as_text(int atom_index, int imol) { 
   
   // It would be cool to flash the residue here.
   // (heh - it is).
   // 
   graphics_info_t g;
   PCAtom picked_atom = g.molecules[imol].atom_sel.atom_selection[atom_index];
   
   g.flash_selection(imol, 
		     picked_atom->residue->seqNum,
		     picked_atom->GetInsCode(),
		     picked_atom->residue->seqNum,
		     picked_atom->GetInsCode(),
		     picked_atom->altLoc,
		     picked_atom->residue->GetChainID());

   PPCAtom atoms;
   int n_atoms;

   picked_atom->residue->GetAtomTable(atoms,n_atoms);
   for (int i=0; i<n_atoms; i++) { 
      std::string segid = atoms[i]->segID;
      std::cout << "(" << imol << ") " 
		<< atoms[i]->name << "/"
		<< atoms[i]->GetModelNum()
		<< "/"
		<< atoms[i]->GetChainID()  << "/"
		<< atoms[i]->GetSeqNum()   << "/"
		<< atoms[i]->GetResName()
		<< ", "
		<< segid
		<< " occ: " 
		<< atoms[i]->occupancy 
		<< " with B-factor: "
		<< atoms[i]->tempFactor
		<< " element: \""
		<< atoms[i]->element
		<< "\""
		<< " at " << "("
		<< atoms[i]->x << "," << atoms[i]->y << "," 
		<< atoms[i]->z << ")" << std::endl;
   }

   // chi angles:
   coot::chi_angles chi_angles(picked_atom->residue, 0);
   std::vector<std::pair<int, float> > chis = chi_angles.get_chi_angles();
   if (chis.size() > 0) {
      std::cout << "   Chi Angles:" << std::endl;
      for (unsigned int ich=0; ich<chis.size(); ich++) {
	 std::cout << "     chi "<< chis[ich].first << ": " << chis[ich].second
		   << " degrees" << std::endl;
      }
   } else {
      std::cout << "No Chi Angles for this residue" << std::endl;
   } 
   
   std::string cmd = "output-residue-info-as-text";
   std::vector<coot::command_arg_t> args;
   args.push_back(atom_index);
   args.push_back(imol);
   add_to_history_typed(cmd, args);
}

// Actually I want to return a scheme object with occ, pos, b-factor info
// 
void
output_atom_info_as_text(int imol, const char *chain_id, int resno,
			 const char *inscode, const char *atname,
			 const char *altconf) {

   if (is_valid_model_molecule(imol)) {
      int index =
	 graphics_info_t::molecules[imol].full_atom_spec_to_atom_index(std::string(chain_id),
							resno,
							std::string(inscode),
							std::string(atname),
							std::string(altconf));
      
      CAtom *atom = graphics_info_t::molecules[imol].atom_sel.atom_selection[index];
      std::cout << "(" << imol << ") " 
		<< atom->name << "/"
		<< atom->GetModelNum()
		<< "/"
		<< atom->GetChainID()  << "/"
		<< atom->GetSeqNum()   << "/"
		<< atom->GetResName()
		<< ", occ: " 
		<< atom->occupancy 
		<< " with B-factor: "
		<< atom->tempFactor
		<< " element: \""
		<< atom->element
		<< "\""
		<< " at " << "("
		<< atom->x << "," << atom->y << "," 
		<< atom->z << ")" << std::endl;
      // chi angles:
      coot::chi_angles chi_angles(atom->residue, 0);
      std::vector<std::pair<int, float> > chis = chi_angles.get_chi_angles();
      if (chis.size() > 0) {
	 std::cout << "   Chi Angles:" << std::endl;
	 for (unsigned int ich=0; ich<chis.size(); ich++) {
	    std::cout << "     chi "<< chis[ich].first << ": " << chis[ich].second
		      << " degrees" << std::endl;
	 }
      } else {
	 std::cout << "No Chi Angles for this residue" << std::endl;
      }
   }
   std::string cmd = "output-atom-info-as-text";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(chain_id);
   args.push_back(resno);
   args.push_back(inscode);
   args.push_back(atname);
   args.push_back(altconf);
   add_to_history_typed(cmd, args);

}

#ifdef USE_GUILE
// (list occ temp-factor element x y z) or #f 
const char *atom_info_string(int imol, const char *chain_id, int resno,
		     const char *ins_code, const char *atname,
		     const char *altconf) {

   const char *r = 0;  // guile/SWIG sees this as #f
   if (is_valid_model_molecule(imol)) {
      int index =
	 graphics_info_t::molecules[imol].full_atom_spec_to_atom_index(std::string(chain_id),
								       resno,
								       std::string(ins_code),
								       std::string(atname),
								       std::string(altconf));
      if (index > -1) { 
	 CAtom *atom = graphics_info_t::molecules[imol].atom_sel.atom_selection[index];

	 // we need the ' because eval needs it.
	 std::string s = "(quote (";
	 s += coot::util::float_to_string(atom->occupancy);
	 s += " ";
	 s += coot::util::float_to_string(atom->tempFactor);
	 s += " ";
	 s += single_quote(atom->element);
	 s += " ";
	 s += coot::util::float_to_string(atom->x);
	 s += " ";
	 s += coot::util::float_to_string(atom->y);
	 s += " ";
	 s += coot::util::float_to_string(atom->z);
	 s += "))";
	 r = new char[s.length() + 1];
	 strcpy((char *)r, s.c_str());
      }
   }
   std::string cmd = "atom-info-string";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(resno);
   args.push_back(coot::util::single_quote(ins_code));
   args.push_back(coot::util::single_quote(atname));
   args.push_back(coot::util::single_quote(altconf));
   add_to_history_typed(cmd, args);

   return r;
}
#endif // USE_GUILE
// BL says:: we return a string in python list compatible format.
// to use it in python you need to eval the string!
#ifdef USE_PYTHON
// "[occ,temp_factor,element,x,y,z]" or 0
const char *atom_info_string_py(int imol, const char *chain_id, int resno,
                     const char *ins_code, const char *atname,
                     const char *altconf) {

   const char *r = 0;  // python/SWIG sees this as False (esp once eval'ed)
   if (is_valid_model_molecule(imol)) {
      int index =
         graphics_info_t::molecules[imol].full_atom_spec_to_atom_index(std::string(chain_id),
                                                                       resno,
                                                                       std::string(ins_code),
                                                                       std::string(atname),
                                                                       std::string(altconf));
      if (index > -1) { 
         CAtom *atom = graphics_info_t::molecules[imol].atom_sel.atom_selection[index];

         // we need the ' because eval needs it.
         std::string s = "[";
         s += coot::util::float_to_string(atom->occupancy);
         s += ",";
         s += coot::util::float_to_string(atom->tempFactor);
         s += ",";
         s += single_quote(atom->element);
         s += ",";
         s += coot::util::float_to_string(atom->x);
         s += ",";
         s += coot::util::float_to_string(atom->y);
         s += ",";
         s += coot::util::float_to_string(atom->z);
         s += "]";
         r = new char[s.length() + 1];
         strcpy((char *)r, s.c_str());
      }
   }
   std::string cmd = "atom_info_string";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(chain_id);
   args.push_back(resno);
   args.push_back(ins_code);
   args.push_back(atname);
   args.push_back(altconf);
   add_to_history_typed(cmd, args);

   return r;
}
#endif // PYTHON

#ifdef USE_GUILE
// output is like this:
// (list
//    (list (list atom-name alt-conf)
//          (list occ temp-fact element)
//          (list x y z)))
// 
SCM residue_info(int imol, const char* chain_id, int resno, const char *ins_code) {

   SCM r = SCM_BOOL(0);
   if (is_valid_model_molecule(imol)) {
      CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int imod = 1;
      
      CModel *model_p = mol->GetModel(imod);
      CChain *chain_p;
      // run over chains of the existing mol
      int nchains = model_p->GetNumberOfChains();
      for (int ichain=0; ichain<nchains; ichain++) {
	 chain_p = model_p->GetChain(ichain);
	 std::string chain_id_mol(chain_p->GetChainID());
	 if (chain_id_mol == std::string(chain_id)) { 
	    int nres = chain_p->GetNumberOfResidues();
	    PCResidue residue_p;
	    CAtom *at;

	    // why use this bizarre contrivance to get a null list for
	    // starting? I must be missing something.
	    SCM all_atoms = SCM_CAR(scm_listofnull);
	    for (int ires=0; ires<nres; ires++) { 
	       residue_p = chain_p->GetResidue(ires);
	       std::string res_ins_code(residue_p->GetInsCode());
	       if (residue_p->GetSeqNum() == resno) { 
		  if (res_ins_code == std::string(ins_code)) {
		     int n_atoms = residue_p->GetNumberOfAtoms();
		     SCM at_info = SCM_BOOL(0);
		     SCM at_pos;
		     SCM at_occ, at_b, at_ele, at_name, at_altconf;
		     SCM at_x, at_y, at_z;
		     for (int iat=0; iat<n_atoms; iat++) {
			at = residue_p->GetAtom(iat);
			at_x  = scm_float2num(at->x);
			at_y  = scm_float2num(at->y);
			at_z  = scm_float2num(at->z);
			at_pos = scm_list_3(at_x, at_y, at_z);
			at_occ = scm_float2num(at->occupancy);
			at_b   = scm_float2num(at->tempFactor);
			at_ele = scm_makfrom0str(at->element);
			at_name = scm_makfrom0str(at->name);
			at_altconf = scm_makfrom0str(at->altLoc);
			SCM compound_name = scm_list_2(at_name, at_altconf);
			SCM compound_attrib = scm_list_3(at_occ, at_b, at_ele);
			at_info = scm_list_3(compound_name, compound_attrib, at_pos);
			all_atoms = scm_cons(at_info, all_atoms);
		     }
		  }
	       }
	    }
	    r = all_atoms;
	 }
      }
   }
   return r;
}
#endif // USE_GUILE
// BL says:: this is my attepmt to code it in python
#ifdef USE_PYTHON
PyObject *residue_info_py(int imol, const char* chain_id, int resno, const char *ins_code) {

   PyObject *r;
   PyObject *all_atoms;
   PyObject *compound_name;
   PyObject *compound_attrib;
   r = Py_False;
   if (is_valid_model_molecule(imol)) {
      CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int imod = 1;
      
      CModel *model_p = mol->GetModel(imod);
      CChain *chain_p;
      // run over chains of the existing mol
      int nchains = model_p->GetNumberOfChains();
      for (int ichain=0; ichain<nchains; ichain++) {
         chain_p = model_p->GetChain(ichain);
         std::string chain_id_mol(chain_p->GetChainID());
         if (chain_id_mol == std::string(chain_id)) { 
            int nres = chain_p->GetNumberOfResidues();
            PCResidue residue_p;
            CAtom *at;

            // why use this bizarre contrivance to get a null list for
            // starting? I must be missing something.
            for (int ires=0; ires<nres; ires++) { 
               residue_p = chain_p->GetResidue(ires);
               std::string res_ins_code(residue_p->GetInsCode());
               if (residue_p->GetSeqNum() == resno) { 
                  if (res_ins_code == std::string(ins_code)) {
                     int n_atoms = residue_p->GetNumberOfAtoms();
                     PyObject *at_info = Py_False;
                     PyObject *at_pos;
                     PyObject *at_occ, *at_b, *at_ele, *at_name, *at_altconf;
                     PyObject *at_x, *at_y, *at_z;
                     all_atoms = PyList_New(n_atoms);
                     for (int iat=0; iat<n_atoms; iat++) {

                        at = residue_p->GetAtom(iat);
                        at_x  = PyFloat_FromDouble(at->x);
                        at_y  = PyFloat_FromDouble(at->y);
                        at_z  = PyFloat_FromDouble(at->z);
                        at_pos = PyList_New(3);
                        PyList_SetItem(at_pos,0,at_x);
                        PyList_SetItem(at_pos,1,at_y);
                        PyList_SetItem(at_pos,2,at_z);

                        at_occ = PyFloat_FromDouble(at->occupancy);
                        at_b   = PyFloat_FromDouble(at->tempFactor);
                        at_ele = PyString_FromString(at->element);
                        at_name = PyString_FromString(at->name);
                        at_altconf = PyString_FromString(at->altLoc);

                        compound_name = PyList_New(2);
                        PyList_SetItem(compound_name, 0 ,at_name);
                        PyList_SetItem(compound_name, 1 ,at_altconf);

                        compound_attrib = PyList_New(3);
                        PyList_SetItem(compound_attrib, 0, at_occ);
                        PyList_SetItem(compound_attrib, 1, at_b);
                        PyList_SetItem(compound_attrib, 2, at_ele);

                        at_info = PyList_New(3);
                        PyList_SetItem(at_info, 0, compound_name);
                        PyList_SetItem(at_info, 1, compound_attrib);
                        PyList_SetItem(at_info, 2, at_pos);

                        PyList_SetItem(all_atoms, iat, at_info);
                     }
                  }
               }
            }
            r = all_atoms;
         }
      }
   }
   return r;
}

#endif //PYTHON


#ifdef USE_GUILE
SCM residue_name(int imol, const char* chain_id, int resno, const char *ins_code) {

   SCM r = SCM_BOOL(0);
   if (is_valid_model_molecule(imol)) {
      CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int imod = 1;
      bool have_resname_flag = 0;
      
      CModel *model_p = mol->GetModel(imod);
      CChain *chain_p;
      // run over chains of the existing mol
      int nchains = model_p->GetNumberOfChains();
      for (int ichain=0; ichain<nchains; ichain++) {
	 chain_p = model_p->GetChain(ichain);
	 std::string chain_id_mol(chain_p->GetChainID());
	 if (chain_id_mol == std::string(chain_id)) { 
	    int nres = chain_p->GetNumberOfResidues();
	    PCResidue residue_p;
	    for (int ires=0; ires<nres; ires++) { 
	       residue_p = chain_p->GetResidue(ires);
	       if (residue_p->GetSeqNum() == resno) { 
		  std::string ins = residue_p->GetInsCode();
		  if (ins == ins_code) {
		     r = scm_makfrom0str(residue_p->GetResName());
		     have_resname_flag = 1;
		     break;
		  }
	       }
	    }
	 }
	 if (have_resname_flag)
	    break;
      }
   }
   return r;
}
#endif // USE_GUILE
#ifdef USE_PYTHON
PyObject *residue_name_py(int imol, const char* chain_id, int resno, const char *ins_code) {

   PyObject *r;
   r = Py_False;
   if (is_valid_model_molecule(imol)) {
      CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int imod = 1;
      bool have_resname_flag = 0;
      
      CModel *model_p = mol->GetModel(imod);
      CChain *chain_p;
      // run over chains of the existing mol
      int nchains = model_p->GetNumberOfChains();
      for (int ichain=0; ichain<nchains; ichain++) {
         chain_p = model_p->GetChain(ichain);
         std::string chain_id_mol(chain_p->GetChainID());
         if (chain_id_mol == std::string(chain_id)) { 
            int nres = chain_p->GetNumberOfResidues();
            PCResidue residue_p;
            for (int ires=0; ires<nres; ires++) { 
               residue_p = chain_p->GetResidue(ires);
               if (residue_p->GetSeqNum() == resno) { 
                  std::string ins = residue_p->GetInsCode();
                  if (ins == ins_code) {
                     r = PyString_FromString(residue_p->GetResName());
                     have_resname_flag = 1;
                     break;
                  }
               }
            }
         }
         if (have_resname_flag)
            break;
      }
   }
   return r;
}
#endif // USE_PYTHON


#ifdef USE_GUILE
// Bernie, no need to pythonize this, it's just to test the return
// values on pressing "next residue" and "previous residue" (you can
// if you wish of course).
//
// Pass the current values, return new values
//
// Hmm maybe these function should pass the atom name too?  Yes they should
SCM goto_next_atom_maybe(const char *chain_id, int resno, const char *ins_code,
			 const char *atom_name) {

   SCM r = SCM_BOOL_F;

   int imol = go_to_atom_molecule_number();
   if (is_valid_model_molecule(imol)) { 

      int atom_index =
	 graphics_info_t::molecules[imol].intelligent_next_atom(chain_id, resno,
								atom_name, ins_code);

      if (atom_index != -1) {
	 CAtom *next_atom = graphics_info_t::molecules[imol].atom_sel.atom_selection[atom_index];

	 std::string next_chain_id  = next_atom->GetChainID();
	 std::string next_atom_name = next_atom->name;
	 int next_residue_number    = next_atom->GetSeqNum();
	 std::string next_ins_code  = next_atom->GetInsCode();

	 r = SCM_EOL;
	 r = scm_cons(scm_makfrom0str(next_atom_name.c_str()), r);
	 r = scm_cons(scm_makfrom0str(next_ins_code.c_str()), r);
	 r = scm_cons(scm_int2num(next_residue_number) ,r);
	 r = scm_cons(scm_makfrom0str(next_chain_id.c_str()), r);
      }
   }
   return r;
}
#endif

#ifdef USE_GUILE
SCM goto_prev_atom_maybe(const char *chain_id, int resno, const char *ins_code,
			 const char *atom_name) {

   SCM r = SCM_BOOL_F;
   int imol = go_to_atom_molecule_number();
   if (is_valid_model_molecule(imol)) { 

      int atom_index =
	 graphics_info_t::molecules[imol].intelligent_previous_atom(chain_id, resno,
								    atom_name, ins_code);

      if (atom_index != -1) {
	 CAtom *next_atom = graphics_info_t::molecules[imol].atom_sel.atom_selection[atom_index];

	 std::string next_chain_id  = next_atom->GetChainID();
	 std::string next_atom_name = next_atom->name;
	 int next_residue_number    = next_atom->GetSeqNum();
	 std::string next_ins_code  = next_atom->GetInsCode();

	 r = SCM_EOL;
	 r = scm_cons(scm_makfrom0str(next_atom_name.c_str()), r);
	 r = scm_cons(scm_makfrom0str(next_ins_code.c_str()), r);
	 r = scm_cons(scm_int2num(next_residue_number) ,r);
	 r = scm_cons(scm_makfrom0str(next_chain_id.c_str()), r);
      }
   }
   return r;
} 
#endif 

// A C++ function interface:
// 
int set_go_to_atom_from_spec(const coot::atom_spec_t &atom_spec) {

   graphics_info_t g;

   g.set_go_to_atom_chain_residue_atom_name(atom_spec.chain.c_str(), 
					    atom_spec.resno,
					    atom_spec.insertion_code.c_str(), 
					    atom_spec.atom_name.c_str(),
					    atom_spec.alt_conf.c_str());

   int success = g.try_centre_from_new_go_to_atom(); 
   if (success)
      update_things_on_move_and_redraw(); 

   return success; 
}

// (is-it-valid? (active-molecule-number spec))
std::pair<bool, std::pair<int, coot::atom_spec_t> > active_atom_spec() {

   coot::atom_spec_t spec;
   bool was_found_flag = 0;
   
   graphics_info_t g;
   float dist_best = 999999999.9;
   int imol_closest = -1;
   CAtom *at_close = 0;
   
   for (int imol=0; imol<graphics_info_t::n_molecules(); imol++) {

      if (is_valid_model_molecule(imol)) {
	 if (graphics_info_t::molecules[imol].is_displayed_p()) { 
	    coot::at_dist_info_t at_info =
	       graphics_info_t::molecules[imol].closest_atom(g.RotationCentre());
	    if (at_info.atom) {
	       if (at_info.dist <= dist_best) {
		  dist_best = at_info.dist;
		  imol_closest = at_info.imol;
		  at_close = at_info.atom;
	       }
	    }
	 }
      }
   }
   if (at_close) {
      spec = coot::atom_spec_t(at_close);
      was_found_flag = 1;
   }

   std::pair<int, coot::atom_spec_t> p1(imol_closest, spec);
   return std::pair<bool, std::pair<int, coot::atom_spec_t> > (was_found_flag, p1);
} 

#ifdef USE_GUILE
//* \brief 
// Return a list of (list imol chain-id resno ins-code atom-name
// alt-conf) for atom that is closest to the screen centre.  If there
// are multiple models with the same coordinates at the screen centre,
// return the attributes of the atom in the highest number molecule
// number.
// 
SCM active_residue() {

   SCM s = SCM_BOOL(0);
   std::pair<bool, std::pair<int, coot::atom_spec_t> > pp = active_atom_spec();

   if (pp.first) {
      s = SCM_EOL;
      s = scm_cons(scm_makfrom0str(pp.second.second.alt_conf.c_str()) , s);
      s = scm_cons(scm_makfrom0str(pp.second.second.atom_name.c_str()) , s);
      s = scm_cons(scm_makfrom0str(pp.second.second.insertion_code.c_str()) , s);
      s = scm_cons(scm_int2num(pp.second.second.resno) , s);
      s = scm_cons(scm_makfrom0str(pp.second.second.chain.c_str()) , s);
      s = scm_cons(scm_int2num(pp.second.first) ,s);
   } 
   return s;
}
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *active_residue_py() {

   PyObject *s;
   s = Py_False;
   std::pair<bool, std::pair<int, coot::atom_spec_t> > pp = active_atom_spec();

   if (pp.first) {
      s = PyList_New(6);
      PyList_SetItem(s, 0, PyInt_FromLong(pp.second.first));
      PyList_SetItem(s, 1, PyString_FromString(pp.second.second.chain.c_str()));
      PyList_SetItem(s, 2, PyInt_FromLong(pp.second.second.resno));
      PyList_SetItem(s, 3, PyString_FromString(pp.second.second.insertion_code.c_str()));
      PyList_SetItem(s, 4, PyString_FromString(pp.second.second.atom_name.c_str()));
      PyList_SetItem(s, 5, PyString_FromString(pp.second.second.alt_conf.c_str()));
   }
   return s;
}
#endif // PYTHON

#ifdef USE_GUILE
SCM closest_atom(int imol) {

   SCM r = SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      coot::at_dist_info_t at_info =
	 graphics_info_t::molecules[imol].closest_atom(g.RotationCentre());
      if (at_info.atom) {
	 r = SCM_EOL;
	 r = scm_cons(scm_double2num(at_info.atom->z), r);
	 r = scm_cons(scm_double2num(at_info.atom->y), r);
	 r = scm_cons(scm_double2num(at_info.atom->x), r);
	 r = scm_cons(scm_makfrom0str(at_info.atom->altLoc), r);
	 r = scm_cons(scm_makfrom0str(at_info.atom->name), r);
	 r = scm_cons(scm_makfrom0str(at_info.atom->GetInsCode()), r);
	 r = scm_cons(scm_int2num(at_info.atom->GetSeqNum()), r);
	 r = scm_cons(scm_makfrom0str(at_info.atom->GetChainID()), r);
	 r = scm_cons(scm_int2num(imol), r);
      }
   }
   return r;
} 
#endif 

#ifdef USE_PYTHON
PyObject *closest_atom_py(int imol) {

   PyObject *r;
   r = Py_False;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      coot::at_dist_info_t at_info =
	 graphics_info_t::molecules[imol].closest_atom(g.RotationCentre());
      if (at_info.atom) {
         r = PyList_New(9);
	 PyList_SetItem(r, 8, PyFloat_FromDouble(at_info.atom->z));
	 PyList_SetItem(r, 7, PyFloat_FromDouble(at_info.atom->y));
	 PyList_SetItem(r, 6, PyFloat_FromDouble(at_info.atom->x));
	 PyList_SetItem(r, 5, PyString_FromString(at_info.atom->altLoc));
	 PyList_SetItem(r, 4, PyString_FromString(at_info.atom->name));
	 PyList_SetItem(r, 3, PyString_FromString(at_info.atom->GetInsCode()));
	 PyList_SetItem(r, 2, PyInt_FromLong(at_info.atom->GetSeqNum()));
	 PyList_SetItem(r, 1, PyString_FromString(at_info.atom->GetChainID()));
	 PyList_SetItem(r, 0, PyInt_FromLong(imol));
      }
   }
   return r;
} 
#endif // USE_PYTHON

/*! \brief update the Go To Atom widget entries to atom closest to
  screen centre. */
void update_go_to_atom_from_current_position() {
   
   std::pair<bool, std::pair<int, coot::atom_spec_t> > pp = active_atom_spec();
   if (pp.first) {
      set_go_to_atom_molecule(pp.second.first);
      set_go_to_atom_chain_residue_atom_name(pp.second.second.chain.c_str(),
					     pp.second.second.resno,
					     pp.second.second.atom_name.c_str());
      update_go_to_atom_window_on_other_molecule_chosen(pp.second.first);
   }
}


#ifdef USE_GUILE
SCM generic_string_vector_to_list_internal(const std::vector<std::string> &v) {

   SCM r = SCM_CAR(scm_listofnull);
   for (int i=v.size()-1; i>=0; i--) {
      r = scm_cons(scm_makfrom0str(v[i].c_str()), r);
   }
   return r; 
}
#endif // USE_GUILE

// BL says:: python version 
#ifdef USE_PYTHON
PyObject *generic_string_vector_to_list_internal_py(const std::vector<std::string> &v) {

  PyObject *r = PyList_New(0);

   r = PyList_New(v.size());
   for (int i=v.size()-1; i>=0; i--) {
      PyList_SetItem(r, i, PyString_FromString(v[i].c_str()));
   }
   return r;
}
#endif // PYTHON

// and the reverse function:
#ifdef USE_GUILE
std::vector<std::string>
generic_list_to_string_vector_internal(SCM l) {
   std::vector<std::string> r;
   SCM l_length_scm = scm_length(l);

#if (SCM_MAJOR_VERSION > 1) || (SCM_MINOR_VERSION > 7)

   int l_length = scm_to_int(l_length_scm);
   for (int i=0; i<l_length; i++) {
      SCM le = scm_list_ref(l, SCM_MAKINUM(i));
      std::string s = scm_to_locale_string(le);
      r.push_back(s);
   } 
   
#else
   
   int l_length = gh_scm2int(l_length_scm);
   for (int i=0; i<l_length; i++) {
      SCM le = scm_list_ref(l, SCM_MAKINUM(i));
      std::string s =  SCM_STRING_CHARS(le);
      r.push_back(s);
   }

#endif

   return r;
}
#endif

#ifdef USE_PYTHON
std::vector<std::string>
generic_list_to_string_vector_internal_py(PyObject *l) {
   std::vector<std::string> r;

   int l_length = PyObject_Length(l);
   for (int i=0; i<l_length; i++) {
      PyObject *le = PyList_GetItem(l, i);
      std::string s = PyString_AsString(le);
      r.push_back(s);
   } 

   return r;
}
   
#endif // USE_PYTHON

#ifdef USE_GUILE
SCM generic_int_vector_to_list_internal(const std::vector<int> &v) {

   SCM r = SCM_EOL;
   for (int i=v.size()-1; i>=0; i--) {
      r = scm_cons(scm_int2num(v[i]), r);
   }
   return r; 
}
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *generic_int_vector_to_list_internal_py(const std::vector<int> &v) {

   PyObject *r;
   r = PyList_New(v.size()); 
   for (int i=v.size()-1; i>=0; i--) {
      PyList_SetItem(r, i, PyInt_FromLong(v[i]));
   }
   return r; 
}
#endif // USE_PYTHON

#ifdef USE_GUILE
SCM rtop_to_scm(const clipper::RTop_orth &rtop) {

   SCM r = SCM_EOL;

   SCM tr_list = SCM_EOL;
   SCM rot_list = SCM_EOL;

   clipper::Mat33<double>  mat = rtop.rot();
   clipper::Vec3<double> trans = rtop.trn();

   tr_list = scm_cons(scm_double2num(trans[2]), tr_list);
   tr_list = scm_cons(scm_double2num(trans[1]), tr_list);
   tr_list = scm_cons(scm_double2num(trans[0]), tr_list);

   rot_list = scm_cons(scm_double2num(mat(2,2)), rot_list);
   rot_list = scm_cons(scm_double2num(mat(2,1)), rot_list);
   rot_list = scm_cons(scm_double2num(mat(2,0)), rot_list);
   rot_list = scm_cons(scm_double2num(mat(1,2)), rot_list);
   rot_list = scm_cons(scm_double2num(mat(1,1)), rot_list);
   rot_list = scm_cons(scm_double2num(mat(1,0)), rot_list);
   rot_list = scm_cons(scm_double2num(mat(0,2)), rot_list);
   rot_list = scm_cons(scm_double2num(mat(0,1)), rot_list);
   rot_list = scm_cons(scm_double2num(mat(0,0)), rot_list);

   r = scm_cons(tr_list, r);
   r = scm_cons(rot_list, r);
   return r;

}
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *rtop_to_python(const clipper::RTop_orth &rtop) {

   PyObject *r;
   PyObject *tr_list;
   PyObject *rot_list;

   r = PyList_New(2);
   tr_list = PyList_New(3);
   rot_list = PyList_New(9);

   clipper::Mat33<double>  mat = rtop.rot();
   clipper::Vec3<double> trans = rtop.trn();

   PyList_SetItem(tr_list, 0, PyFloat_FromDouble(trans[0]));
   PyList_SetItem(tr_list, 1, PyFloat_FromDouble(trans[1]));
   PyList_SetItem(tr_list, 2, PyFloat_FromDouble(trans[2]));

   PyList_SetItem(rot_list, 0, PyFloat_FromDouble(mat(0,0)));
   PyList_SetItem(rot_list, 1, PyFloat_FromDouble(mat(0,1)));
   PyList_SetItem(rot_list, 2, PyFloat_FromDouble(mat(0,2)));
   PyList_SetItem(rot_list, 3, PyFloat_FromDouble(mat(1,0)));
   PyList_SetItem(rot_list, 4, PyFloat_FromDouble(mat(1,1)));
   PyList_SetItem(rot_list, 5, PyFloat_FromDouble(mat(1,2)));
   PyList_SetItem(rot_list, 6, PyFloat_FromDouble(mat(2,0)));
   PyList_SetItem(rot_list, 7, PyFloat_FromDouble(mat(2,1)));
   PyList_SetItem(rot_list, 8, PyFloat_FromDouble(mat(2,2)));

// BL says:: or maybe the other way round
   PyList_SetItem(r, 0, rot_list);
   PyList_SetItem(r, 1, tr_list);
   return r;

}
#endif // USE_PYTHON

#ifdef USE_GUILE
// get the symmetry operators strings for the given molecule
//
/*! \brief Return as a list of strings the symmetry operators of the
  given molecule. If imol is a not a valid molecule, return an empty
  list.*/
// 
SCM get_symmetry(int imol) {

   SCM r = SCM_CAR(scm_listofnull);
   if (is_valid_model_molecule(imol) ||
       is_valid_map_molecule(imol)) {
      std::vector<std::string> symop_list =
	 graphics_info_t::molecules[imol].get_symop_strings();
      r = generic_string_vector_to_list_internal(symop_list);
   }
   return r; 
} 
#endif 

// BL says:: python's get_symmetry:
#ifdef USE_PYTHON
PyObject *get_symmetry_py(int imol) {

   PyObject *r;
   r = PyList_New(0);
   if (is_valid_model_molecule(imol) ||
       is_valid_map_molecule(imol)) {
      std::vector<std::string> symop_list =
         graphics_info_t::molecules[imol].get_symop_strings();
      r = generic_string_vector_to_list_internal_py(symop_list);
   }
   return r;
}
#endif //PYTHON


void residue_info_apply_all_checkbutton_toggled() {

} 


void apply_residue_info_changes(GtkWidget *widget) {
   graphics_info_t g;
   g.apply_residue_info_changes(widget);
   graphics_draw();
} 

void do_distance_define() {

   std::cout << "Click on 2 atoms: " << std::endl;
   graphics_info_t g;
   g.pick_cursor_maybe();
   g.in_distance_define = 1;
   g.pick_pending_flag = 1;

} 

void do_angle_define() {

   std::cout << "Click on 3 atoms: " << std::endl;
   graphics_info_t g;
   g.pick_cursor_maybe();
   g.in_angle_define = 1;
   g.pick_pending_flag = 1;

} 

void do_torsion_define() {

   std::cout << "Click on 4 atoms: " << std::endl;
   graphics_info_t g;
   g.pick_cursor_maybe();
   g.in_torsion_define = 1;
   g.pick_pending_flag = 1;

} 

void clear_simple_distances() {
   graphics_info_t g;
   g.clear_simple_distances();
   g.normal_cursor();
   std::string cmd = "clear-simple-distances";
   std::vector<coot::command_arg_t> args;
   add_to_history_typed(cmd, args);
}

void clear_last_simple_distance() { 

   graphics_info_t g;
   g.clear_last_simple_distance();
   g.normal_cursor();
   std::string cmd = "clear-last-simple-distance";
   std::vector<coot::command_arg_t> args;
   add_to_history_typed(cmd, args);
} 

void store_geometry_dialog(GtkWidget *w) { 

   graphics_info_t g;
   g.geometry_dialog = w;
   if (w) { 
      gtk_window_set_transient_for(GTK_WINDOW(w),
				   GTK_WINDOW(lookup_widget(g.glarea, "window1")));
   }
}


void clear_residue_info_edit_list() {

   graphics_info_t::residue_info_edits->resize(0);
   std::string cmd = "clear-residue-info-edit-list";
   std::vector<coot::command_arg_t> args;
   add_to_history_typed(cmd, args);
}


/*  ----------------------------------------------------------------------- */
/*                  residue environment                                      */
/*  ----------------------------------------------------------------------- */
void fill_environment_widget(GtkWidget *widget) {

   GtkWidget *entry;
   char *text = (char *) malloc(100);
   graphics_info_t g;

   entry = lookup_widget(widget, "environment_distance_min_entry");
   snprintf(text, 99, "%-5.1f", g.environment_min_distance);
   gtk_entry_set_text(GTK_ENTRY(entry), text);
   
   entry = lookup_widget(widget, "environment_distance_max_entry");
   snprintf(text, 99, "%-5.1f" ,g.environment_max_distance);
   gtk_entry_set_text(GTK_ENTRY(entry), text);
   free(text);

   GtkWidget *toggle_button;
   toggle_button = lookup_widget(widget, "environment_distance_checkbutton");
   
   if (g.environment_show_distances == 1) {
      // we have to (temporarily) set the flag to 0 because the
      // set_active creates an event which causes
      // toggle_environment_show_distances to run (and thus turn off
      // distances if they were allowed to remain here at 1 (on).
      // Strange but true.
      g.environment_show_distances = 0;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button), 1);
      // std::cout << "filling: button is active" << std::endl;
   } else {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button), 0);
      // std::cout << "filling: button is inactive" << std::endl;
   } 
}

// Called when the OK button of the environment distances dialog is clicked 
// (just before it is destroyed).
// 
void execute_environment_settings(GtkWidget *widget) {
   
   GtkWidget *entry;
   float val;
   graphics_info_t g;

   entry = lookup_widget(widget, "environment_distance_min_entry");
   const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
   val = atof(text);
   if (val < 0 || val > 1000) {
      g.environment_min_distance = 2.2;
      std::cout <<  "nonsense value for limit using "
		<< g.environment_min_distance << std::endl;
   } else {
      g.environment_min_distance = val;
   } 

   entry = lookup_widget(widget, "environment_distance_max_entry");
   text = gtk_entry_get_text(GTK_ENTRY(entry));
   val = atof(text);
   if (val < 0 || val > 1000) {
      g.environment_max_distance = 3.2;
      std::cout <<  "nonsense value for limit using "
		<< g.environment_max_distance << std::endl;
   } else {
      g.environment_max_distance = val;
   }

   if (g.environment_max_distance < g.environment_min_distance) {
      float tmp = g.environment_max_distance;
      g.environment_max_distance = g.environment_min_distance;
      g.environment_min_distance = tmp;
   }

   GtkWidget *label_check_button;
   label_check_button = lookup_widget(widget, "environment_distance_label_atom_checkbutton");
   if (GTK_TOGGLE_BUTTON(label_check_button)->active) {
      g.environment_distance_label_atom = 1;
   }

   // not sure that this is necessary now that the toggle function is
   // active
   std::pair<int, int> r =  g.get_closest_atom();
   if (r.first >= 0) { 
      g.update_environment_distances_maybe(r.first, r.second);
   }
   graphics_draw();
}

void set_show_environment_distances(int state) {

   graphics_info_t g;
   g.environment_show_distances = state;
   if (state) {
      std::pair<int, int> r =  g.get_closest_atom();
      if (r.first >= 0) { 
	 g.update_environment_distances_maybe(r.first, r.second);
      }
   }
   graphics_draw();
}

int show_environment_distances_state() {
   return graphics_info_t::environment_show_distances;
}

/*! \brief min and max distances for the environment distances */
void set_environment_distances_distance_limits(float min_dist, float max_dist) {

   graphics_info_t::environment_min_distance = min_dist;
   graphics_info_t::environment_max_distance = max_dist;
} 



void toggle_environment_show_distances(GtkToggleButton *button) {

   graphics_info_t g;

   //    if (g.environment_show_distances == 0) {
   
   GtkWidget *hbox = lookup_widget(GTK_WIDGET(button),
				   "environment_distance_distances_frame");
   GtkWidget *label_atom_check_button =
      lookup_widget(GTK_WIDGET(button),
		    "environment_distance_label_atom_checkbutton");
   
   if (button->active) { 
      // std::cout << "toggled evironment distances on" << std::endl;
      g.environment_show_distances = 1;
      gtk_widget_set_sensitive(hbox, TRUE);
      gtk_widget_set_sensitive(label_atom_check_button, TRUE);

      // 
      std::pair<int, int> r =  g.get_closest_atom();
//       std::cout << "DEBUG:: got close info: " 
// 		<< r.first << " " << r.second << std::endl;
      if (r.first >= 0) { 
	 g.update_environment_distances_maybe(r.first, r.second);
	 graphics_draw();
      }
      
   } else {
      // std::cout << "toggled evironment distances off" << std::endl;
      g.environment_show_distances = 0;
      gtk_widget_set_sensitive(hbox, FALSE);
      gtk_widget_set_sensitive(label_atom_check_button, FALSE);
   }
}

/* a graphics_info_t function wrapper: */
void residue_info_release_memory(GtkWidget *widget) { 

   graphics_info_t g;
   g.residue_info_release_memory(widget);

}

void unset_residue_info_widget() { 
   graphics_info_t g;
   g.residue_info_dialog = NULL; 
} 


/*  ----------------------------------------------------------------------- */
/*                  pointer distances                                      */
/*  ----------------------------------------------------------------------- */
void fill_pointer_distances_widget(GtkWidget *widget) {

   GtkWidget *min_entry   = lookup_widget(widget, "pointer_distances_min_dist_entry");
   GtkWidget *max_entry   = lookup_widget(widget, "pointer_distances_max_dist_entry");
   GtkWidget *checkbutton = lookup_widget(widget, "pointer_distances_checkbutton");
   GtkWidget *frame       = lookup_widget(widget, "pointer_distances_frame");

   float min_dist = graphics_info_t::pointer_min_dist;
   float max_dist = graphics_info_t::pointer_max_dist;
   
   gtk_entry_set_text(GTK_ENTRY(min_entry),
		      graphics_info_t::float_to_string(min_dist).c_str());
   gtk_entry_set_text(GTK_ENTRY(max_entry),
		      graphics_info_t::float_to_string(max_dist).c_str());

   if (graphics_info_t::show_pointer_distances_flag) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), TRUE);
      gtk_widget_set_sensitive(frame, TRUE);
   } else {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), FALSE);
      gtk_widget_set_sensitive(frame, FALSE);
   }

}

void execute_pointer_distances_settings(GtkWidget *widget) {

   GtkWidget *min_entry   = lookup_widget(widget, "pointer_distances_min_dist_entry");
   GtkWidget *max_entry   = lookup_widget(widget, "pointer_distances_max_dist_entry");
   // GtkWidget *checkbutton = lookup_widget(widget, "pointer_distances_checkbutton");

   float min_dist = 0.0;
   float max_dist = 0.0;

   float t;

   const gchar *tt = gtk_entry_get_text(GTK_ENTRY(min_entry));
   t = atof(tt);

   if (t >= 0.0 & t < 999.9)
      min_dist = t;

   tt = gtk_entry_get_text(GTK_ENTRY(max_entry));
   t = atof(tt);

   if (t >= 0.0 & t < 999.9)
      max_dist = t;

   graphics_info_t::pointer_max_dist = max_dist;
   graphics_info_t::pointer_min_dist = min_dist;

   
}


void toggle_pointer_distances_show_distances(GtkToggleButton *togglebutton) {

   GtkWidget *frame = lookup_widget(GTK_WIDGET(togglebutton),
				    "pointer_distances_frame");
   if (togglebutton->active) {
      set_show_pointer_distances(1);
      gtk_widget_set_sensitive(frame, TRUE);
   } else {
      set_show_pointer_distances(0);
      gtk_widget_set_sensitive(frame, FALSE);
   }
   
}


void set_show_pointer_distances(int istate) {

   // Use the graphics_info_t's pointer min and max dist

   std::cout << "in set_show_pointer_distances: state: "
	     << istate << std::endl;
   
   if (istate == 0) { 
      graphics_info_t::show_pointer_distances_flag = 0;
      graphics_info_t g;
      g.clear_pointer_distances();
   } else {
      graphics_info_t::show_pointer_distances_flag = 1;
      graphics_info_t g;
      // std::cout << "in set_show_pointer_distances: making distances.." << std::endl;
      g.make_pointer_distance_objects();
      // std::cout << "in set_show_pointer_distances: done making distances.." << std::endl;
   }
   graphics_draw();
   graphics_info_t::residue_info_edits->resize(0);
   std::string cmd = "set-show-pointer-distances";
   std::vector<coot::command_arg_t> args;
   args.push_back(istate);
   add_to_history_typed(cmd, args);
}


void fill_single_map_properties_dialog(GtkWidget *window, int imol) { 

   GtkWidget *cell_text = lookup_widget(window, "single_map_properties_cell_text");
   GtkWidget *spgr_text = lookup_widget(window, "single_map_properties_sg_text");

   std::string cell_text_string;
   std::string spgr_text_string;

   cell_text_string = graphics_info_t::molecules[imol].cell_text_with_embeded_newline();
   spgr_text_string = "   ";
   spgr_text_string += graphics_info_t::molecules[imol].xmap_list[0].spacegroup().descr().symbol_hm();
   spgr_text_string += "  [";
   spgr_text_string += graphics_info_t::molecules[imol].xmap_list[0].spacegroup().descr().symbol_hall();
   spgr_text_string += "]";


   gtk_label_set_text(GTK_LABEL(cell_text), cell_text_string.c_str());
   gtk_label_set_text(GTK_LABEL(spgr_text), spgr_text_string.c_str());
} 

/*  ----------------------------------------------------------------------- */
/*                  miguels orientation axes matrix                         */
/*  ----------------------------------------------------------------------- */

void
set_axis_orientation_matrix(float m11, float m12, float m13,
			    float m21, float m22, float m23,
			    float m31, float m32, float m33) {

   graphics_info_t::axes_orientation =
      GL_matrix(m11, m12, m13, m21, m22, m23, m31, m32, m33);

   std::string cmd = "set-axis-orientation-matrix";
   std::vector<coot::command_arg_t> args;
   args.push_back(m11);
   args.push_back(m12);
   args.push_back(m12);
   args.push_back(m21);
   args.push_back(m22);
   args.push_back(m23);
   args.push_back(m31);
   args.push_back(m32);
   args.push_back(m33);
   add_to_history_typed(cmd, args);
}

void
set_axis_orientation_matrix_usage(int state) {

   graphics_info_t::use_axes_orientation_matrix_flag = state;
   graphics_draw();
   std::string cmd = "set-axis-orientation-matrix-usage";
   std::vector<coot::command_arg_t> args;
   args.push_back(state);
   add_to_history_typed(cmd, args);

} 



/*  ----------------------------------------------------------------------- */
/*                  dynamic map                                             */
/*  ----------------------------------------------------------------------- */
void toggle_dynamic_map_sampling() {
   graphics_info_t g;
   // std::cout << "toggling from " << g.dynamic_map_resampling << std::endl;
   if (g.dynamic_map_resampling) {
      g.dynamic_map_resampling = 0;
   } else {
      g.dynamic_map_resampling = 1;
   }
   std::string cmd = "toggle-dynamic-map-sampling";
   std::vector<coot::command_arg_t> args;
   add_to_history_typed(cmd, args);
}


void toggle_dynamic_map_display_size() {
   graphics_info_t g;
   // std::cout << "toggling from " << g.dynamic_map_size_display << std::endl;
   if (g.dynamic_map_size_display) {
      g.dynamic_map_size_display = 0;
   } else {
      g.dynamic_map_size_display = 1;
   }
} 

void   set_map_dynamic_map_sampling_checkbutton(GtkWidget *checkbutton) {

   graphics_info_t g;
   if (g.dynamic_map_resampling) {
      g.dynamic_map_resampling = 0;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), 1);
   }
}

void
set_map_dynamic_map_display_size_checkbutton(GtkWidget *checkbutton) {

   graphics_info_t g;
   if (g.dynamic_map_size_display) {
      g.dynamic_map_size_display = 0; 
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), 1);
   }
} 

void
set_dynamic_map_size_display_on() {
   graphics_info_t::dynamic_map_size_display = 1;
} 
void
set_dynamic_map_size_display_off() {
   graphics_info_t::dynamic_map_size_display = 0;
} 
int
get_dynamic_map_size_display(){
   int ret = graphics_info_t::dynamic_map_size_display;
   return ret;
}
void
set_dynamic_map_sampling_on() {
   graphics_info_t::dynamic_map_resampling = 1;
}
void
set_dynamic_map_sampling_off() {
   graphics_info_t::dynamic_map_resampling = 0;
}
int
get_dynamic_map_sampling(){
   int ret = graphics_info_t::dynamic_map_resampling;
   return ret;
}

void
set_dynamic_map_zoom_offset(int i) {
   graphics_info_t::dynamic_map_zoom_offset = i;
} 




/*  ------------------------------------------------------------------------ */
/*                         history                                           */
/*  ------------------------------------------------------------------------ */



void add_to_history(const std::vector<std::string> &command_strings) {
   // something
   graphics_info_t g;
   g.add_history_command(command_strings);

   if (g.console_display_commands.display_commands_flag) { 

      char esc = 27;
      // std::string esc = "esc";
      if (g.console_display_commands.hilight_flag) {
	 // std::cout << esc << "[34m";
	 std::cout << esc << "[1m";
      } else {
	 std::cout << "INFO:: Command: ";
      }

      // Make it colourful?
      if (g.console_display_commands.hilight_colour_flag)
	 std::cout << esc << "[3"
		   << g.console_display_commands.colour_prefix << "m";

      std::cout << graphics_info_t::schemize_command_strings(command_strings);
      
      if (g.console_display_commands.hilight_flag) {// hilight off
	 std::cout << esc << "[0m"; // reset
      }
      std::cout << std::endl;
   }

#ifdef USE_MYSQL_DATABASE

   add_to_database(command_strings);
#endif
}

void add_to_history_typed(const std::string &command,
			  const std::vector<coot::command_arg_t> &args) {

   std::vector<std::string> command_strings;

   command_strings.push_back(command);
   for (unsigned int i=0; i<args.size(); i++)
      command_strings.push_back(args[i].as_string());

   add_to_history(command_strings);
}

void add_to_history_simple(const std::string &s) {

   std::vector<std::string> command_strings;
   command_strings.push_back(s);
   add_to_history(command_strings);
} 

std::string
single_quote(const std::string &s) {
   std::string r("\"");
   r += s;
   r += "\"";
   return r;
} 

std::string pythonize_command_name(const std::string &s) {

   std::string ss;
   for (unsigned int i=0; i<s.length(); i++) {
      if (s[i] == '-') {
	 ss += '_';
      } else {
	 ss += s[i];	 
      }
   }
   return ss;
} 

std::string schemize_command_name(const std::string &s) {

   std::string ss;
   for (unsigned int i=0; i<s.length(); i++) {
      if (s[i] == '_') {
	 ss += '-';
      } else {
	 ss += s[i];	 
      }
   }
   return ss;
}

#ifdef USE_MYSQL_DATABASE
int db_query_insert(const std::string &insert_string) {

   int v = -1;
   if (graphics_info_t::mysql) {

      time_t timep = time(0);
      long int li = timep;
      
      clipper::String query("insert into session");
      query += " (userid, sessionid, command, commandnumber, timeasint)";
      query += " values ('";
      query += graphics_info_t::db_userid_username.first;
      query += "', '";
      query += graphics_info_t::sessionid;
      query += "', '";
      query += insert_string;
      query += "', ";
      query += graphics_info_t::int_to_string(graphics_info_t::query_number);
      query += ", ";
      query += coot::util::long_int_to_string(li);
      query += ") ; ";

//       query = "insert into session ";
//       query += "(userid, sessionid, command, commandnumber) values ";
//       query += "('pemsley', 'sesh', 'xxx', ";
//       query += graphics_info_t::int_to_string(graphics_info_t::query_number);
//       query += ") ;";
      
//       std::cout << "query: " << query << std::endl;
      unsigned long length = query.length();
      v = mysql_real_query(graphics_info_t::mysql, query.c_str(), length);
      if (v != 0) {
	 if (v == CR_COMMANDS_OUT_OF_SYNC)
	    std::cout << "WARNING:: MYSQL Commands executed in an"
		      << " improper order" << std::endl;
	 if (v == CR_SERVER_GONE_ERROR) 
	    std::cout << "WARNING:: MYSQL Server gone!"
		      << std::endl;
	 if (v == CR_SERVER_LOST) 
	    std::cout << "WARNING:: MYSQL Server lost during query!"
		      << std::endl;
	 if (v == CR_UNKNOWN_ERROR) 
	    std::cout << "WARNING:: MYSQL Server transaction had "
		      << "an uknown error!" << std::endl;
	 std::cout << "history: mysql_real_query returned " << v
		   << std::endl;
      }
      graphics_info_t::query_number++;
   }
   return v;
}
#endif // USE_MYSQL_DATABASE

void add_to_database(const std::vector<std::string> &command_strings) {

#ifdef USE_MYSQL_DATABASE
   std::string insert_string =
      graphics_info_t::schemize_command_strings(command_strings);
   db_query_insert(insert_string);
#endif // USE_MYSQL_DATABASE

}


#ifdef USE_MYSQL_DATABASE
// 
void db_finish_up() {

   db_query_insert(";#finish");

} 
#endif // USE_MYSQL_DATABASE


/*  ----------------------------------------------------------------------- */
/*                         History/scripting                                */
/*  ----------------------------------------------------------------------- */

void print_all_history_in_scheme() {

   graphics_info_t g;
   std::vector<std::vector<std::string> > ls = g.history_list.history_list();
   for (unsigned int i=0; i<ls.size(); i++)
      std::cout << i << "  " << graphics_info_t::schemize_command_strings(ls[i]) << "\n";

   add_to_history_simple("print-all-history-in-scheme");

}

void print_all_history_in_python() {

   graphics_info_t g;
   std::vector<std::vector<std::string> > ls = g.history_list.history_list();
   for (unsigned int i=0; i<ls.size(); i++)
      std::cout << i << "  " << graphics_info_t::pythonize_command_strings(ls[i]) << "\n";
   add_to_history_simple("print-all-history-in-python");
}

/*! \brief set a flag to show the text command equivalent of gui
  commands in the console as they happen. 

  1 for on, 0 for off. */
void set_console_display_commands_state(short int istate) {

   graphics_info_t::console_display_commands.display_commands_flag = istate;
}

void set_console_display_commands_hilights(short int bold_flag, short int colour_flag, int colour_index) {

   graphics_info_t g;
   g.console_display_commands.hilight_flag = bold_flag;
   g.console_display_commands.hilight_colour_flag = colour_flag;
   g.console_display_commands.colour_prefix = colour_index;
}



std::string languagize_command(const std::vector<std::string> &command_parts) {

   short int language = 0; 
#ifdef USE_PYTHON
#ifdef USE_GUILE
   language = coot::STATE_SCM;
#else   
   language = coot::STATE_PYTHON;
#endif
#endif

#ifdef USE_GUILE
   language = 1;
#endif

   std::string s;
   if (language) {
      if (language == coot::STATE_PYTHON)
	 s = graphics_info_t::pythonize_command_strings(command_parts);
      if (language == coot::STATE_SCM)
	 s = graphics_info_t::schemize_command_strings(command_parts);
   }
   return s; 
}


/*  ------------------------------------------------------------------------ */
/*                     state (a graphics_info thing)                         */
/*  ------------------------------------------------------------------------ */

void
save_state() {
   graphics_info_t g;
   g.save_state();
   add_to_history_simple("save-state");
}

void
save_state_file(const char *filename) {
   graphics_info_t g;
   g.save_state_file(std::string(filename));
   std::string cmd = "save-state-file";
   std::vector<coot::command_arg_t> args;
   args.push_back(single_quote(filename));
   add_to_history_typed(cmd, args);
}


/*  ------------------------------------------------------------------------ */
/*                     resolution                                            */
/*  ------------------------------------------------------------------------ */

/* Return negative number on error, otherwise resolution in A (eg. 2.0) */
float data_resolution(int imol) {

   float r = -1;
   if (is_valid_map_molecule(imol)) {
      r = graphics_info_t::molecules[imol].data_resolution();
   }
   std::string cmd = "data-resolution";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   return r;
}

/*  ------------------------------------------------------------------------ */
/*                     resolution                                            */
/*  ------------------------------------------------------------------------ */

void solid_surface(int imap, short int on_off_flag) {

   if (is_valid_map_molecule(imap)) {
      graphics_info_t::molecules[imap].do_solid_surface_for_density(on_off_flag);
   }
   graphics_draw();
} 



/*  ------------------------------------------------------------------------ */
/*                     residue exists?                                       */
/*  ------------------------------------------------------------------------ */

int does_residue_exist_p(int imol, char *chain_id, int resno, char *inscode) {

   int istate = 0;
   if (is_valid_model_molecule(imol)) {
      istate = graphics_info_t::molecules[imol].does_residue_exist_p(std::string(chain_id),
								     resno,
								     std::string(inscode));
   }
   std::string cmd = "does-residue-exist-p";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(chain_id);
   args.push_back(resno);
   args.push_back(inscode);
   add_to_history_typed(cmd, args);
   return istate;
}

/*  ------------------------------------------------------------------------ */
/*                         Parameters from map:                              */
/*  ------------------------------------------------------------------------ */
/*! \brief return the mtz file that was use to generate the map

  return 0 when there is no mtz file associated with that map (it was
  generated from a CCP4 map file say). */
const char *mtz_hklin_for_map(int imol_map) {

   std::string mtz;

   if (is_valid_map_molecule(imol_map)) {
      mtz = graphics_info_t::molecules[imol_map].save_mtz_file_name;
   }
   std::string cmd = "mtz-hklin-for-map";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol_map);
   add_to_history_typed(cmd, args);
   return mtz.c_str();
}

/*! \brief return the FP column in the file that was use to generate
  the map

  return 0 when there is no mtz file associated with that map (it was
  generated from a CCP4 map file say). */
const char *mtz_fp_for_map(int imol_map) {

   std::string fp;
   if (is_valid_map_molecule(imol_map)) {
      std::cout << imol_map << " Is valid map molecule" << std::endl;
      fp = graphics_info_t::molecules[imol_map].save_f_col;
   }
   std::string cmd = "mtz-fp-for-map";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol_map);
   add_to_history_typed(cmd, args);
   return fp.c_str();
} 

/*! \brief return the phases column in mtz file that was use to generate
  the map

  return 0 when there is no mtz file associated with that map (it was
  generated from a CCP4 map file say). */
const char *mtz_phi_for_map(int imol_map) {

   std::string phi;
   if (is_valid_map_molecule(imol_map)) {
      phi = graphics_info_t::molecules[imol_map].save_phi_col;
   }
   std::string cmd = "mtz-phi-for-map";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol_map);
   add_to_history_typed(cmd, args);
   return phi.c_str();
}

/*! \brief return the weight column in the mtz file that was use to
  generate the map

  return 0 when there is no mtz file associated with that map (it was
  generated from a CCP4 map file say) or no weights were used. */
const char *mtz_weight_for_map(int imol_map) {

   std::string weight;
   if (is_valid_map_molecule(imol_map)) {
      weight = graphics_info_t::molecules[imol_map].save_weight_col;
   }
   std::string cmd = "mtz-weight-for-map";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol_map);
   add_to_history_typed(cmd, args);
   return weight.c_str();
}

/*! \brief return flag for whether weights were used that was use to
  generate the map

  return 0 when no weights were used or there is no mtz file
  associated with that map. */
short int mtz_use_weight_for_map(int imol_map) {

   short int i;
   if (is_valid_map_molecule(imol_map)) {
      i = graphics_info_t::molecules[imol_map].save_use_weights;
   }
   std::string cmd = "mtz-use-weight-for-map";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol_map);
   add_to_history_typed(cmd, args);
   return i;
} 


/*! \brief Put text at x,y,z  */
// use should be given access to colour and size.
int place_text(const char *text, float x, float y, float z, int size) {

   int handle = graphics_info_t::generic_texts_p->size();
   std::string s(text); 
   coot::generic_text_object_t o(s, handle, x, y, z); 
   graphics_info_t::generic_texts_p->push_back(o);
   //   return graphics_info_t::generic_text->size() -1; // the index of the
	  					    // thing we just
						    // pushed.
   std::string cmd = "place-text";
   std::vector<coot::command_arg_t> args;
   args.push_back(text);
   args.push_back(x);
   args.push_back(y);
   args.push_back(z);
   args.push_back(size);
   add_to_history_typed(cmd, args);

   return handle; // same value as above.
} 

void remove_text(int text_handle) {

   std::vector<coot::generic_text_object_t>::iterator it;
   for (it = graphics_info_t::generic_texts_p->begin();
	it != graphics_info_t::generic_texts_p->end();
	it++) {
      if (it->handle == text_handle) {
	 graphics_info_t::generic_texts_p->erase(it);
	 break;
      }
   }
   std::string cmd = "remove-text";
   std::vector<coot::command_arg_t> args;
   args.push_back(text_handle);
   add_to_history_typed(cmd, args);
   graphics_draw();
} 

/*  ----------------------------------------------------------------------- */
/*                         Dictionaries                                     */
/*  ----------------------------------------------------------------------- */
/*! \brief return a list of all the dictionaries read */

#ifdef USE_GUILE
SCM dictionaries_read() {

   return generic_string_vector_to_list_internal(*graphics_info_t::cif_dictionary_filename_vec);
}
#endif

// BL says:: python's fucn
#ifdef USE_PYTHON
PyObject *dictionaries_read_py() {

   return generic_string_vector_to_list_internal_py(*graphics_info_t::cif_dictionary_filename_vec);
}
#endif // PYTHON


/*  ----------------------------------------------------------------------- */
/*                         Restraints                                       */
/*  ----------------------------------------------------------------------- */
#ifdef USE_GUILE
SCM monomer_restraints(const char *monomer_type) {

   SCM r = SCM_BOOL_F;

   graphics_info_t g;
   std::pair<short int, coot::dictionary_residue_restraints_t> p =
      g.Geom_p()->get_monomer_restraints(monomer_type);
   if (p.first) {

      coot::dictionary_residue_restraints_t restraints = p.second;
      r = SCM_EOL;

      // ------------------ chem_comp -------------------------
      coot::dict_chem_comp_t info = restraints.residue_info;
      SCM chem_comp_scm = SCM_EOL;
      chem_comp_scm = scm_cons(scm_makfrom0str(info.comp_id.c_str()),           chem_comp_scm);
      chem_comp_scm = scm_cons(scm_makfrom0str(info.three_letter_code.c_str()), chem_comp_scm);
      chem_comp_scm = scm_cons(scm_makfrom0str(info.name.c_str()),              chem_comp_scm);
      chem_comp_scm = scm_cons(scm_makfrom0str(info.group.c_str()),             chem_comp_scm);
      chem_comp_scm = scm_cons(scm_int2num(info.number_atoms_all),              chem_comp_scm);
      chem_comp_scm = scm_cons(scm_int2num(info.number_atoms_nh),               chem_comp_scm);
      chem_comp_scm = scm_cons(scm_makfrom0str(info.description_level.c_str()), chem_comp_scm);
      chem_comp_scm = scm_reverse(chem_comp_scm);
      SCM chem_comp_container = SCM_EOL;
      chem_comp_container = scm_cons(chem_comp_scm, chem_comp_container);
      chem_comp_container = scm_cons(scm_makfrom0str("_chem_comp"), chem_comp_container);

      // ------------------ chem_comp_atom -------------------------
      std::vector<coot::dict_atom> atom_info = restraints.atom_info;
      int n_atoms = atom_info.size();
      SCM atom_info_list = SCM_EOL;
      for (int iat=0; iat<n_atoms; iat++) { 
	 SCM atom_attributes_list = SCM_EOL;
	 atom_attributes_list = scm_cons(scm_makfrom0str(atom_info[iat].atom_id_4c.c_str()), atom_attributes_list);
	 atom_attributes_list = scm_cons(scm_makfrom0str(atom_info[iat].type_symbol.c_str()), atom_attributes_list);
	 atom_attributes_list = scm_cons(scm_makfrom0str(atom_info[iat].type_energy.c_str()), atom_attributes_list);
	 atom_attributes_list = scm_cons(scm_double2num(atom_info[iat].partial_charge.second), atom_attributes_list);
	 SCM partial_flag = SCM_BOOL_F;
	 if (atom_info[iat].partial_charge.first)
	    partial_flag = SCM_BOOL_T;
	 atom_attributes_list = scm_cons(partial_flag, atom_attributes_list);
	 atom_attributes_list = scm_reverse(atom_attributes_list);
	 atom_info_list = scm_cons(atom_attributes_list, atom_info_list);
      }
      atom_info_list = scm_reverse(atom_info_list);
      SCM atom_info_list_container = SCM_EOL;
      atom_info_list_container = scm_cons(atom_info_list, atom_info_list_container);
      atom_info_list_container = scm_cons(scm_makfrom0str("_chem_comp_atom"), atom_info_list_container);


      // ------------------ Bonds -------------------------
      SCM bond_restraint_list = SCM_EOL;
      
      for (unsigned int ibond=0; ibond<restraints.bond_restraint.size(); ibond++) {
	 coot::dict_bond_restraint_t bond_restraint = restraints.bond_restraint[ibond];
	 std::string a1 = bond_restraint.atom_id_1_4c();
	 std::string a2 = bond_restraint.atom_id_2_4c();
	 double d   = bond_restraint.dist();
	 double esd = bond_restraint.esd();
	 SCM bond_restraint_scm = SCM_EOL;
	 bond_restraint_scm = scm_cons(scm_double2num(esd), bond_restraint_scm);
	 bond_restraint_scm = scm_cons(scm_double2num(d),   bond_restraint_scm);
	 bond_restraint_scm = scm_cons(scm_makfrom0str(a2.c_str()),   bond_restraint_scm);
	 bond_restraint_scm = scm_cons(scm_makfrom0str(a1.c_str()),   bond_restraint_scm);
	 bond_restraint_list = scm_cons(bond_restraint_scm, bond_restraint_list);
      }
      SCM bond_restraints_container = SCM_EOL;
      bond_restraints_container = scm_cons(bond_restraint_list, bond_restraints_container);
      bond_restraints_container = scm_cons(scm_makfrom0str("_chem_comp_bond"), bond_restraints_container);

      // ------------------ Angles -------------------------
      SCM angle_restraint_list = SCM_EOL;
      for (unsigned int iangle=0; iangle<restraints.angle_restraint.size(); iangle++) {
	 coot::dict_angle_restraint_t angle_restraint = restraints.angle_restraint[iangle];
	 std::string a1 = angle_restraint.atom_id_1_4c();
	 std::string a2 = angle_restraint.atom_id_2_4c();
	 std::string a3 = angle_restraint.atom_id_3_4c();
	 double d   = angle_restraint.angle();
	 double esd = angle_restraint.esd();
	 SCM angle_restraint_scm = SCM_EOL;
	 angle_restraint_scm = scm_cons(scm_double2num(esd), angle_restraint_scm);
	 angle_restraint_scm = scm_cons(scm_double2num(d),   angle_restraint_scm);
	 angle_restraint_scm = scm_cons(scm_makfrom0str(a3.c_str()),   angle_restraint_scm);
	 angle_restraint_scm = scm_cons(scm_makfrom0str(a2.c_str()),   angle_restraint_scm);
	 angle_restraint_scm = scm_cons(scm_makfrom0str(a1.c_str()),   angle_restraint_scm);
	 angle_restraint_list = scm_cons(angle_restraint_scm, angle_restraint_list);
      }
      SCM angle_restraints_container = SCM_EOL;
      angle_restraints_container = scm_cons(angle_restraint_list, angle_restraints_container);
      angle_restraints_container = scm_cons(scm_makfrom0str("_chem_comp_angle"), angle_restraints_container);

      // ------------------ Torsions -------------------------
      SCM torsion_restraint_list = SCM_EOL;
      for (unsigned int itorsion=0; itorsion<restraints.torsion_restraint.size(); itorsion++) {
	 coot::dict_torsion_restraint_t torsion_restraint = restraints.torsion_restraint[itorsion];
	 std::string a1 = torsion_restraint.atom_id_1_4c();
	 std::string a2 = torsion_restraint.atom_id_2_4c();
	 std::string a3 = torsion_restraint.atom_id_3_4c();
	 std::string a4 = torsion_restraint.atom_id_4_4c();
	 double tor  = torsion_restraint.angle();
	 double esd = torsion_restraint.esd();
	 int period = torsion_restraint.periodicity();
	 SCM torsion_restraint_scm = SCM_EOL;
	 torsion_restraint_scm = scm_cons(SCM_MAKINUM(period), torsion_restraint_scm);
	 torsion_restraint_scm = scm_cons(scm_double2num(esd), torsion_restraint_scm);
	 torsion_restraint_scm = scm_cons(scm_double2num(tor), torsion_restraint_scm);
	 torsion_restraint_scm = scm_cons(scm_makfrom0str(a4.c_str()),   torsion_restraint_scm);
	 torsion_restraint_scm = scm_cons(scm_makfrom0str(a3.c_str()),   torsion_restraint_scm);
	 torsion_restraint_scm = scm_cons(scm_makfrom0str(a2.c_str()),   torsion_restraint_scm);
	 torsion_restraint_scm = scm_cons(scm_makfrom0str(a1.c_str()),   torsion_restraint_scm);
	 torsion_restraint_list = scm_cons(torsion_restraint_scm, torsion_restraint_list);
      }
      SCM torsion_restraints_container = SCM_EOL;
      torsion_restraints_container = scm_cons(torsion_restraint_list, torsion_restraints_container);
      torsion_restraints_container = scm_cons(scm_makfrom0str("_chem_comp_tor"), torsion_restraints_container);


      // ------------------ Planes -------------------------
      SCM plane_restraint_list = SCM_EOL;
      for (unsigned int iplane=0; iplane<restraints.plane_restraint.size(); iplane++) {
	 coot::dict_plane_restraint_t plane_restraint = restraints.plane_restraint[iplane];
	 SCM atom_list = SCM_EOL;
	 for (int iat=0; iat<plane_restraint.n_atoms(); iat++) { 
	    std::string at = plane_restraint[iat];
	    atom_list = scm_cons(scm_makfrom0str(at.c_str()), atom_list);
	 }
	 atom_list = scm_reverse(atom_list);

	 double esd = plane_restraint.dist_esd();
	 SCM plane_id_scm = scm_makfrom0str(plane_restraint.plane_id.c_str());

	 SCM plane_restraint_scm = SCM_EOL;
	 plane_restraint_scm = scm_cons(scm_double2num(esd), plane_restraint_scm);
	 plane_restraint_scm = scm_cons(atom_list, plane_restraint_scm);
	 plane_restraint_scm = scm_cons(plane_id_scm, plane_restraint_scm);
	 plane_restraint_list = scm_cons(plane_restraint_scm, plane_restraint_list);
      }
      SCM plane_restraints_container = SCM_EOL;
      plane_restraints_container = scm_cons(plane_restraint_list, plane_restraints_container);
      plane_restraints_container = scm_cons(scm_makfrom0str("_chem_comp_plane_atom"),
					    plane_restraints_container);


      // ------------------ Chirals -------------------------
      SCM chiral_restraint_list = SCM_EOL;
      for (unsigned int ichiral=0; ichiral<restraints.chiral_restraint.size(); ichiral++) {
	 coot::dict_chiral_restraint_t chiral_restraint = restraints.chiral_restraint[ichiral];

	 std::string a1 = chiral_restraint.atom_id_1_4c();
	 std::string a2 = chiral_restraint.atom_id_2_4c();
	 std::string a3 = chiral_restraint.atom_id_3_4c();
	 std::string ac = chiral_restraint.atom_id_c_4c();
	 std::string chiral_id = chiral_restraint.Chiral_Id();
	 int vol_sign = chiral_restraint.volume_sign;

	 double esd = chiral_restraint.volume_sigma();
	 // int volume_sign = chiral_restraint.volume_sign;
	 SCM chiral_restraint_scm = SCM_EOL;
	 chiral_restraint_scm = scm_cons(scm_double2num(esd), chiral_restraint_scm);
	 chiral_restraint_scm = scm_cons(scm_int2num(vol_sign), chiral_restraint_scm);
	 chiral_restraint_scm = scm_cons(scm_makfrom0str(a3.c_str()), chiral_restraint_scm);
	 chiral_restraint_scm = scm_cons(scm_makfrom0str(a2.c_str()), chiral_restraint_scm);
	 chiral_restraint_scm = scm_cons(scm_makfrom0str(a1.c_str()), chiral_restraint_scm);
	 chiral_restraint_scm = scm_cons(scm_makfrom0str(ac.c_str()), chiral_restraint_scm);
	 chiral_restraint_scm = scm_cons(scm_makfrom0str(chiral_id.c_str()), chiral_restraint_scm);
	 chiral_restraint_list = scm_cons(chiral_restraint_scm, chiral_restraint_list);
      }
      SCM chiral_restraints_container = SCM_EOL;
      chiral_restraints_container = scm_cons(chiral_restraint_list, chiral_restraints_container);
      chiral_restraints_container = scm_cons(scm_makfrom0str("_chem_comp_chir"), chiral_restraints_container);

      
      r = scm_cons( chiral_restraints_container, r);
      r = scm_cons(  plane_restraints_container, r);
      r = scm_cons(torsion_restraints_container, r);
      r = scm_cons(  angle_restraints_container, r);
      r = scm_cons(   bond_restraints_container, r);
      r = scm_cons(    atom_info_list_container, r);
      r = scm_cons(         chem_comp_container, r);

   }
   return r;
} 
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *monomer_restraints_py(const char *monomer_type) {

   PyObject *r;
   r = Py_False;

   graphics_info_t g;
   std::pair<short int, coot::dictionary_residue_restraints_t> p =
      g.Geom_p()->get_monomer_restraints(monomer_type);
   if (!p.first) {
      std::cout << "WARNING:: can't find " << monomer_type << " in monomer dictionary"
		<< std::endl;
   } else {

      r = PyDict_New();
	 
      coot::dictionary_residue_restraints_t restraints = p.second;

      // ------------------ chem_comp -------------------------
      coot::dict_chem_comp_t info = restraints.residue_info;
      
      PyObject *chem_comp_py = PyList_New(7);
      PyList_SetItem(chem_comp_py, 0, PyString_FromString(info.comp_id.c_str()));
      PyList_SetItem(chem_comp_py, 1, PyString_FromString(info.three_letter_code.c_str()));
      PyList_SetItem(chem_comp_py, 2, PyString_FromString(info.name.c_str()));
      PyList_SetItem(chem_comp_py, 3, PyString_FromString(info.group.c_str()));
      PyList_SetItem(chem_comp_py, 4, PyInt_FromLong(info.number_atoms_all));
      PyList_SetItem(chem_comp_py, 5, PyInt_FromLong(info.number_atoms_nh));
      PyList_SetItem(chem_comp_py, 6, PyString_FromString(info.description_level.c_str()));
      
      // Put chem_comp_py into a dictionary?
      PyDict_SetItem(r, PyString_FromString("_chem_comp"), chem_comp_py);

      
      // ------------------ chem_comp_atom -------------------------
      std::vector<coot::dict_atom> atom_info = restraints.atom_info;
      int n_atoms = atom_info.size();
      PyObject *atom_info_list = PyList_New(n_atoms);
      for (int iat=0; iat<n_atoms; iat++) { 
	 PyObject *atom_attributes_list = PyList_New(5);
	 PyList_SetItem(atom_attributes_list, 0, PyString_FromString(atom_info[iat].atom_id_4c.c_str()));
	 PyList_SetItem(atom_attributes_list, 1, PyString_FromString(atom_info[iat].type_symbol.c_str()));
	 PyList_SetItem(atom_attributes_list, 2, PyString_FromString(atom_info[iat].type_energy.c_str()));
	 PyList_SetItem(atom_attributes_list, 3, PyFloat_FromDouble(atom_info[iat].partial_charge.second));
	 PyObject *flag = Py_False;
	 if (atom_info[iat].partial_charge.first)
	    flag = Py_True;
	 PyList_SetItem(atom_attributes_list, 4, flag);
	 PyList_SetItem(atom_info_list, iat, atom_attributes_list);
      }

      PyDict_SetItem(r, PyString_FromString("_chem_comp_atom"), atom_info_list);

      // ------------------ Bonds -------------------------
      PyObject *bond_restraint_list = PyList_New(restraints.bond_restraint.size());
      for (unsigned int ibond=0; ibond<restraints.bond_restraint.size(); ibond++) {
	 std::string a1 = restraints.bond_restraint[ibond].atom_id_1_4c();
	 std::string a2 = restraints.bond_restraint[ibond].atom_id_2_4c();
	 double d   = restraints.bond_restraint[ibond].dist();
	 double esd = restraints.bond_restraint[ibond].esd();
	 PyObject *bond_restraint = PyList_New(4);
	 PyList_SetItem(bond_restraint, 0, PyString_FromString(a1.c_str()));
	 PyList_SetItem(bond_restraint, 1, PyString_FromString(a2.c_str()));
	 PyList_SetItem(bond_restraint, 2, PyFloat_FromDouble(d));
	 PyList_SetItem(bond_restraint, 3, PyFloat_FromDouble(esd));
	 PyList_SetItem(bond_restraint_list, ibond, bond_restraint);
      }

      PyDict_SetItem(r, PyString_FromString("_chem_comp_bond"), bond_restraint_list);


      // ------------------ Angles -------------------------
      PyObject *angle_restraint_list = PyList_New(restraints.angle_restraint.size());
      for (unsigned int iangle=0; iangle<restraints.angle_restraint.size(); iangle++) {
	 std::string a1 = restraints.angle_restraint[iangle].atom_id_1_4c();
	 std::string a2 = restraints.angle_restraint[iangle].atom_id_2_4c();
	 std::string a3 = restraints.angle_restraint[iangle].atom_id_3_4c();
	 double d   = restraints.angle_restraint[iangle].angle();
	 double esd = restraints.angle_restraint[iangle].esd();
	 PyObject *angle_restraint = PyList_New(5);
	 PyList_SetItem(angle_restraint, 0, PyString_FromString(a1.c_str()));
	 PyList_SetItem(angle_restraint, 1, PyString_FromString(a2.c_str()));
	 PyList_SetItem(angle_restraint, 2, PyString_FromString(a3.c_str()));
	 PyList_SetItem(angle_restraint, 3, PyFloat_FromDouble(d));
	 PyList_SetItem(angle_restraint, 4, PyFloat_FromDouble(esd));
	 PyList_SetItem(angle_restraint_list, iangle, angle_restraint);
      }

      PyDict_SetItem(r, PyString_FromString("_chem_comp_angle"), angle_restraint_list);

      
      // ------------------ Torsions -------------------------
      PyObject *torsion_restraint_list = PyList_New(restraints.torsion_restraint.size());
      for (unsigned int itorsion=0; itorsion<restraints.torsion_restraint.size(); itorsion++) {
	 std::string a1 = restraints.torsion_restraint[itorsion].atom_id_1_4c();
	 std::string a2 = restraints.torsion_restraint[itorsion].atom_id_2_4c();
	 std::string a3 = restraints.torsion_restraint[itorsion].atom_id_3_4c();
	 std::string a4 = restraints.torsion_restraint[itorsion].atom_id_4_4c();
	 double tor  = restraints.torsion_restraint[itorsion].angle();
	 double esd = restraints.torsion_restraint[itorsion].esd();
	 int period = restraints.torsion_restraint[itorsion].periodicity();
	 PyObject *torsion_restraint = PyList_New(7);
	 PyList_SetItem(torsion_restraint, 0, PyString_FromString(a1.c_str()));
	 PyList_SetItem(torsion_restraint, 1, PyString_FromString(a2.c_str()));
	 PyList_SetItem(torsion_restraint, 2, PyString_FromString(a3.c_str()));
	 PyList_SetItem(torsion_restraint, 3, PyString_FromString(a4.c_str()));
	 PyList_SetItem(torsion_restraint, 4, PyFloat_FromDouble(tor));
	 PyList_SetItem(torsion_restraint, 5, PyFloat_FromDouble(esd));
	 PyList_SetItem(torsion_restraint, 6, PyInt_FromLong(period));
	 PyList_SetItem(torsion_restraint_list, itorsion, torsion_restraint);
      }

      PyDict_SetItem(r, PyString_FromString("_chem_comp_tor"), torsion_restraint_list);

      // ------------------ Planes -------------------------
      PyObject *plane_restraints_list = PyList_New(restraints.plane_restraint.size());
      for (unsigned int iplane=0; iplane<restraints.plane_restraint.size(); iplane++) {
	 PyObject *atom_list = PyList_New(restraints.plane_restraint[iplane].n_atoms());
	 for (int iat=0; iat<restraints.plane_restraint[iplane].n_atoms(); iat++) { 
	    std::string at = restraints.plane_restraint[iplane][iat];
	    PyList_SetItem(atom_list, iat, PyString_FromString(at.c_str()));
	 }
	 double esd = restraints.plane_restraint[iplane].dist_esd();
	 PyObject *plane_restraint = PyList_New(3);
	 PyList_SetItem(plane_restraint, 0, PyString_FromString(restraints.plane_restraint[iplane].plane_id.c_str()));
	 PyList_SetItem(plane_restraint, 1, atom_list);
	 PyList_SetItem(plane_restraint, 2, PyFloat_FromDouble(esd));
	 PyList_SetItem(plane_restraints_list, iplane, plane_restraint);
      }

      PyDict_SetItem(r, PyString_FromString("_chem_comp_plane_atom"), plane_restraints_list);

      // ------------------ Chirals -------------------------
      PyObject *chiral_restraint_list = PyList_New(restraints.chiral_restraint.size());
      for (unsigned int ichiral=0; ichiral<restraints.chiral_restraint.size(); ichiral++) {
	 
	 std::string a1 = restraints.chiral_restraint[ichiral].atom_id_1_4c();
	 std::string a2 = restraints.chiral_restraint[ichiral].atom_id_2_4c();
	 std::string a3 = restraints.chiral_restraint[ichiral].atom_id_3_4c();
	 std::string ac = restraints.chiral_restraint[ichiral].atom_id_c_4c();
	 std::string chiral_id = restraints.chiral_restraint[ichiral].Chiral_Id();

	 double esd = restraints.chiral_restraint[ichiral].volume_sigma();
	 int volume_sign = restraints.chiral_restraint[ichiral].volume_sign;
	 PyObject *chiral_restraint = PyList_New(7);
	 PyList_SetItem(chiral_restraint, 0, PyString_FromString(chiral_id.c_str()));
	 PyList_SetItem(chiral_restraint, 1, PyString_FromString(ac.c_str()));
	 PyList_SetItem(chiral_restraint, 2, PyString_FromString(a1.c_str()));
	 PyList_SetItem(chiral_restraint, 3, PyString_FromString(a2.c_str()));
	 PyList_SetItem(chiral_restraint, 4, PyString_FromString(a3.c_str()));
	 PyList_SetItem(chiral_restraint, 5, PyInt_FromLong(volume_sign));
	 PyList_SetItem(chiral_restraint, 6, PyFloat_FromDouble(esd));
	 PyList_SetItem(chiral_restraint_list, ichiral, chiral_restraint);
      }

      PyDict_SetItem(r, PyString_FromString("_chem_comp_chir"), chiral_restraint_list);
   }
   return r;
}
#endif // USE_PYTHON

#ifdef USE_GUILE
SCM set_monomer_restraints(const char *monomer_type, SCM restraints) {

   SCM retval = SCM_BOOL_F;

   if (!scm_list_p(restraints)) {
      std::cout << " Failed to read restraints - not a list" << std::endl;
   } else {

      std::vector<coot::dict_bond_restraint_t> bond_restraints;
      std::vector<coot::dict_angle_restraint_t> angle_restraints;
      std::vector<coot::dict_torsion_restraint_t> torsion_restraints;
      std::vector<coot::dict_chiral_restraint_t> chiral_restraints;
      std::vector<coot::dict_plane_restraint_t> plane_restraints;
      std::vector<coot::dict_atom> atoms;
      coot::dict_chem_comp_t residue_info;

      SCM restraints_length_scm = scm_length(restraints);
      int restraints_length = scm_to_int(restraints_length_scm);
      if (restraints_length > 0) {
	 for (int i_rest_type=0; i_rest_type<restraints_length; i_rest_type++) {
	    SCM rest_container = scm_list_ref(restraints, SCM_MAKINUM(i_rest_type));
	    if (scm_list_p(rest_container)) {
	       SCM rest_container_length_scm = scm_length(rest_container);
	       int rest_container_length = scm_to_int(rest_container_length_scm);
	       if (rest_container_length == 2) {
		  SCM restraints_type_scm = scm_car(rest_container);
		  if (scm_string_p(restraints_type_scm)) {
		     std::string restraints_type = scm_to_locale_string(restraints_type_scm);

		     if (restraints_type == "_chem_comp") {
			SCM chem_comp_info_scm = scm_list_ref(rest_container, SCM_MAKINUM(1));
			SCM chem_comp_info_length_scm = scm_length(chem_comp_info_scm);
			int chem_comp_info_length = scm_to_int(chem_comp_info_length_scm);

			if (chem_comp_info_length == 7) {
			   SCM  comp_id_scm = scm_list_ref(chem_comp_info_scm, SCM_MAKINUM(0));
			   SCM      tlc_scm = scm_list_ref(chem_comp_info_scm, SCM_MAKINUM(1));
			   SCM     name_scm = scm_list_ref(chem_comp_info_scm, SCM_MAKINUM(2));
			   SCM    group_scm = scm_list_ref(chem_comp_info_scm, SCM_MAKINUM(3));
			   SCM      noa_scm = scm_list_ref(chem_comp_info_scm, SCM_MAKINUM(4));
			   SCM    nonha_scm = scm_list_ref(chem_comp_info_scm, SCM_MAKINUM(5));
			   SCM desc_lev_scm = scm_list_ref(chem_comp_info_scm, SCM_MAKINUM(6));
			   if (scm_is_true(scm_string_p(comp_id_scm)) &&
			       scm_is_true(scm_string_p(tlc_scm)) &&
			       scm_is_true(scm_string_p(name_scm)) &&
			       scm_is_true(scm_string_p(group_scm)) &&
			       scm_is_true(scm_number_p(noa_scm)) &&
			       scm_is_true(scm_number_p(nonha_scm)) &&
			       scm_is_true(scm_string_p(desc_lev_scm))) {
			      std::string comp_id = scm_to_locale_string(comp_id_scm);
			      std::string     tlc = scm_to_locale_string(tlc_scm);
			      std::string    name = scm_to_locale_string(name_scm);
			      std::string   group = scm_to_locale_string(group_scm);
			      std::string des_lev = scm_to_locale_string(desc_lev_scm);
			      int no_of_atoms = scm_to_int(noa_scm);
			      int no_of_non_H_atoms = scm_to_int(nonha_scm);
			      coot::dict_chem_comp_t n(comp_id, tlc, name, group,
						       no_of_atoms, no_of_non_H_atoms,
						       des_lev);
			      residue_info = n;
			   }
			} 
		     }

		     if (restraints_type == "_chem_comp_atom") {
			SCM chem_comp_atoms = scm_list_ref(rest_container, SCM_MAKINUM(1));
			SCM chem_comp_atoms_length_scm = scm_length(chem_comp_atoms);
			int chem_comp_atoms_length = scm_to_int(chem_comp_atoms_length_scm);

			for (int iat=0; iat<chem_comp_atoms_length; iat++) {
			   SCM chem_comp_atom_scm = scm_list_ref(chem_comp_atoms, SCM_MAKINUM(iat));
			   SCM chem_comp_atom_length_scm = scm_length(chem_comp_atom_scm);
			   int chem_comp_atom_length = scm_to_int(chem_comp_atom_length_scm);
			   
			   if (chem_comp_atom_length == 5) { 
			      SCM atom_id_scm  = scm_list_ref(chem_comp_atom_scm, SCM_MAKINUM(0));
			      SCM element_scm  = scm_list_ref(chem_comp_atom_scm, SCM_MAKINUM(1));
			      SCM energy_scm   = scm_list_ref(chem_comp_atom_scm, SCM_MAKINUM(2));
			      SCM partial_charge_scm = scm_list_ref(chem_comp_atom_scm, SCM_MAKINUM(3));
			      SCM valid_pc_scm = scm_list_ref(chem_comp_atom_scm, SCM_MAKINUM(4));

			      if (scm_string_p(atom_id_scm) && scm_string_p(element_scm) &&
				  scm_number_p(partial_charge_scm)) {
				 std::string atom_id(scm_to_locale_string(atom_id_scm));
				 std::string element(scm_to_locale_string(element_scm));
				 std::string energy(scm_to_locale_string(energy_scm));
				 float partial_charge = scm_to_double(partial_charge_scm);
				 short int valid_partial_charge = 1;
				 if SCM_FALSEP(valid_pc_scm)
				    valid_partial_charge = 0;
				 coot::dict_atom at(atom_id, atom_id, element, energy, 
						    std::pair<bool, float>(valid_partial_charge,
									   partial_charge));

				 atoms.push_back(at);
			      }
			   }
			}
			
		     }
		     

		     if (restraints_type == "_chem_comp_bond") {
			SCM bond_restraints_list_scm = scm_list_ref(rest_container, SCM_MAKINUM(1));
			SCM bond_restraints_list_length_scm = scm_length(bond_restraints_list_scm);
			int bond_restraints_list_length = scm_to_int(bond_restraints_list_length_scm);

			for (int ibr=0; ibr<bond_restraints_list_length; ibr++) {
			   SCM bond_restraint = scm_list_ref(bond_restraints_list_scm, SCM_MAKINUM(ibr));
			   SCM bond_restraint_length_scm = scm_length(bond_restraint);
			   int bond_restraint_length = scm_to_int(bond_restraint_length_scm);

			   if (bond_restraint_length == 4) {
			      SCM atom_1_scm = scm_list_ref(bond_restraint, SCM_MAKINUM(0));
			      SCM atom_2_scm = scm_list_ref(bond_restraint, SCM_MAKINUM(1));
			      SCM dist_scm   = scm_list_ref(bond_restraint, SCM_MAKINUM(2));
			      SCM esd_scm    = scm_list_ref(bond_restraint, SCM_MAKINUM(3));
			      if (scm_string_p(atom_1_scm) && scm_string_p(atom_2_scm) &&
				  scm_number_p(dist_scm) && scm_number_p(esd_scm)) {
				 std::string atom_1 = scm_to_locale_string(atom_1_scm);
				 std::string atom_2 = scm_to_locale_string(atom_2_scm);
				 double dist        = scm_to_double(dist_scm);
				 double esd         = scm_to_double(esd_scm);
				 coot::dict_bond_restraint_t rest(atom_1, atom_2, "unknown", dist, esd);
				 bond_restraints.push_back(rest);
			      }
			   }
			}
		     }

		     if (restraints_type == "_chem_comp_angle") {
			SCM angle_restraints_list = scm_list_ref(rest_container, SCM_MAKINUM(1));
			SCM angle_restraints_list_length_scm = scm_length(angle_restraints_list);
			int angle_restraints_list_length = scm_to_int(angle_restraints_list_length_scm);

			for (int iar=0; iar<angle_restraints_list_length; iar++) {
			   SCM angle_restraint = scm_list_ref(angle_restraints_list, SCM_MAKINUM(iar));
			   SCM angle_restraint_length_scm = scm_length(angle_restraint);
			   int angle_restraint_length = scm_to_int(angle_restraint_length_scm);

			   if (angle_restraint_length == 5) {
			      SCM atom_1_scm = scm_list_ref(angle_restraint, SCM_MAKINUM(0));
			      SCM atom_2_scm = scm_list_ref(angle_restraint, SCM_MAKINUM(1));
			      SCM atom_3_scm = scm_list_ref(angle_restraint, SCM_MAKINUM(2));
			      SCM angle_scm  = scm_list_ref(angle_restraint, SCM_MAKINUM(3));
			      SCM esd_scm    = scm_list_ref(angle_restraint, SCM_MAKINUM(4));
			      if (scm_string_p(atom_1_scm) && scm_string_p(atom_2_scm) &&
				  scm_string_p(atom_3_scm) &&
				  scm_number_p(angle_scm) && scm_number_p(esd_scm)) {
				 std::string atom_1 = scm_to_locale_string(atom_1_scm);
				 std::string atom_2 = scm_to_locale_string(atom_2_scm);
				 std::string atom_3 = scm_to_locale_string(atom_3_scm);
				 double angle       = scm_to_double(angle_scm);
				 double esd         = scm_to_double(esd_scm);
				 coot::dict_angle_restraint_t rest(atom_1, atom_2, atom_3, angle, esd);
				 angle_restraints.push_back(rest);
			      }
			   }
			}
		     }


		     if (restraints_type == "_chem_comp_tor") {
			SCM torsion_restraints_list = scm_list_ref(rest_container, SCM_MAKINUM(1));
			SCM torsion_restraints_list_length_scm = scm_length(torsion_restraints_list);
			int torsion_restraints_list_length = scm_to_int(torsion_restraints_list_length_scm);

			for (int itr=0; itr<torsion_restraints_list_length; itr++) {
			   SCM torsion_restraint = scm_list_ref(torsion_restraints_list, SCM_MAKINUM(itr));
			   SCM torsion_restraint_length_scm = scm_length(torsion_restraint);
			   int torsion_restraint_length = scm_to_int(torsion_restraint_length_scm);
			   
			   if (torsion_restraint_length == 7) {
			      SCM atom_1_scm   = scm_list_ref(torsion_restraint, SCM_MAKINUM(0));
			      SCM atom_2_scm   = scm_list_ref(torsion_restraint, SCM_MAKINUM(1));
			      SCM atom_3_scm   = scm_list_ref(torsion_restraint, SCM_MAKINUM(2));
			      SCM atom_4_scm   = scm_list_ref(torsion_restraint, SCM_MAKINUM(3));
			      SCM torsion_scm  = scm_list_ref(torsion_restraint, SCM_MAKINUM(4));
			      SCM esd_scm      = scm_list_ref(torsion_restraint, SCM_MAKINUM(5));
			      SCM period_scm   = scm_list_ref(torsion_restraint, SCM_MAKINUM(6));
			      if (scm_string_p(atom_1_scm) && scm_string_p(atom_2_scm) &&
				  scm_string_p(atom_3_scm) && scm_string_p(atom_4_scm) &&
				  scm_number_p(torsion_scm) && scm_number_p(esd_scm) && 
				  scm_number_p(period_scm)) {
				 std::string atom_1 = scm_to_locale_string(atom_1_scm);
				 std::string atom_2 = scm_to_locale_string(atom_2_scm);
				 std::string atom_3 = scm_to_locale_string(atom_3_scm);
				 std::string atom_4 = scm_to_locale_string(atom_4_scm);
				 double torsion       = scm_to_double(torsion_scm);
				 double esd         = scm_to_double(esd_scm);
				 int period         = scm_to_int(period_scm);
				 coot::dict_torsion_restraint_t rest(atom_1, atom_2, atom_3, atom_4,
								     torsion, esd, period);
				 torsion_restraints.push_back(rest);
			      }
			   }
			}
		     }
		     
		     if (restraints_type == "_chem_comp_plane_atom") {
			SCM plane_restraints_list = scm_list_ref(rest_container, SCM_MAKINUM(1));
			SCM plane_restraints_list_length_scm = scm_length(plane_restraints_list);
			int plane_restraints_list_length = scm_to_int(plane_restraints_list_length_scm);

			for (int ipr=0; ipr<plane_restraints_list_length; ipr++) {
			   SCM plane_restraint = scm_list_ref(plane_restraints_list, SCM_MAKINUM(ipr));
			   SCM plane_restraint_length_scm = scm_length(plane_restraint);
			   int plane_restraint_length = scm_to_int(plane_restraint_length_scm);

			   if (plane_restraint_length == 3) {
			      std::vector<SCM> atoms;
			      SCM plane_id_scm   = scm_list_ref(plane_restraint, SCM_MAKINUM(0));
			      SCM esd_scm        = scm_list_ref(plane_restraint, SCM_MAKINUM(2));
			      SCM atom_list_scm  = scm_list_ref(plane_restraint, SCM_MAKINUM(1));
			      SCM atom_list_length_scm = scm_length(atom_list_scm);
			      int atom_list_length = scm_to_int(atom_list_length_scm);
			      bool atoms_pass = 1;
			      for (int iat=0; iat<atom_list_length; iat++) { 
				 SCM atom_scm   = scm_list_ref(plane_restraint, SCM_MAKINUM(0));
				 atoms.push_back(atom_scm);
				 if (!scm_string_p(atom_scm))
				    atoms_pass = 0;
			      }
			   
			      if (atoms_pass && scm_string_p(plane_id_scm) &&  scm_number_p(esd_scm)) { 
				 std::vector<std::string> atom_names;
				 for (unsigned int i=0; i<atoms.size(); i++)
				    atom_names.push_back(std::string(scm_to_locale_string(atoms[i])));

				 std::string plane_id = scm_to_locale_string(plane_id_scm);
				 double esd           = scm_to_double(esd_scm);
				 if (atom_names.size() > 0) { 
				    coot::dict_plane_restraint_t rest(plane_id, atom_names[0], esd);
				    for (unsigned int i=1; i<atom_names.size(); i++)
				       rest.push_back_atom(atom_names[i]);
				    plane_restraints.push_back(rest);
				 }
			      }
			   }
			}
		     }
		     
		     
		     if (restraints_type == "_chem_comp_chir") {
			SCM chiral_restraints_list = scm_list_ref(rest_container, SCM_MAKINUM(1));
			SCM chiral_restraints_list_length_scm = scm_length(chiral_restraints_list);
			int chiral_restraints_list_length = scm_to_int(chiral_restraints_list_length_scm);

			for (int icr=0; icr<chiral_restraints_list_length; icr++) {
			   SCM chiral_restraint = scm_list_ref(chiral_restraints_list, SCM_MAKINUM(icr));
			   SCM chiral_restraint_length_scm = scm_length(chiral_restraint);
			   int chiral_restraint_length = scm_to_int(chiral_restraint_length_scm);

			   if (chiral_restraint_length == 7) {
			      SCM chiral_id_scm= scm_list_ref(chiral_restraint, SCM_MAKINUM(0));
			      SCM atom_c_scm   = scm_list_ref(chiral_restraint, SCM_MAKINUM(1));
			      SCM atom_1_scm   = scm_list_ref(chiral_restraint, SCM_MAKINUM(2));
			      SCM atom_2_scm   = scm_list_ref(chiral_restraint, SCM_MAKINUM(3));
			      SCM atom_3_scm   = scm_list_ref(chiral_restraint, SCM_MAKINUM(4));
			      SCM chiral_vol_sign_scm = scm_list_ref(chiral_restraint, SCM_MAKINUM(5));
			      SCM esd_scm      = scm_list_ref(chiral_restraint, SCM_MAKINUM(6));
			      if (scm_string_p(atom_1_scm) && scm_string_p(atom_2_scm) &&
				  scm_string_p(atom_3_scm) && scm_string_p(atom_c_scm) &&
				  scm_number_p(esd_scm)) {
				 std::string chiral_id = scm_to_locale_string(chiral_id_scm);
				 std::string atom_1 = scm_to_locale_string(atom_1_scm);
				 std::string atom_2 = scm_to_locale_string(atom_2_scm);
				 std::string atom_3 = scm_to_locale_string(atom_3_scm);
				 std::string atom_c = scm_to_locale_string(atom_c_scm);
				 double esd         = scm_to_double(esd_scm);
				 int chiral_vol_sign= scm_to_int(chiral_vol_sign_scm);
				 coot::dict_chiral_restraint_t rest(chiral_id,
								    atom_c, atom_1, atom_2, atom_3,
								    chiral_vol_sign);
				 
				 chiral_restraints.push_back(rest);
			      }
			   }
			}
		     }
		  }
	       }
	    }
	 }
      }

//       std::cout << "Found " <<    bond_restraints.size() << "   bond  restraints" << std::endl;
//       std::cout << "Found " <<   angle_restraints.size() << "   angle restraints" << std::endl;
//       std::cout << "Found " << torsion_restraints.size() << " torsion restraints" << std::endl;
//       std::cout << "Found " <<   plane_restraints.size() << "   plane restraints" << std::endl;
//       std::cout << "Found " <<  chiral_restraints.size() << "  chiral restraints" << std::endl;

      coot::dictionary_residue_restraints_t monomer_restraints(monomer_type, 1);
      monomer_restraints.bond_restraint    = bond_restraints;
      monomer_restraints.angle_restraint   = angle_restraints;
      monomer_restraints.torsion_restraint = torsion_restraints;
      monomer_restraints.chiral_restraint  = chiral_restraints;
      monomer_restraints.plane_restraint   = plane_restraints;
      monomer_restraints.residue_info      = residue_info;
      monomer_restraints.atom_info         = atoms; 

      graphics_info_t g;
      bool s = g.Geom_p()->replace_monomer_restraints(monomer_type, monomer_restraints);
      if (s)
	 retval = SCM_BOOL_T;
   }

   return retval;
}
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *set_monomer_restraints_py(const char *monomer_type, PyObject *restraints) {

   PyObject *retval = Py_False;

   if (!PyDict_Check(restraints)) {
      std::cout << " Failed to read restraints - not a list" << std::endl;
   } else {

      std::vector<coot::dict_bond_restraint_t> bond_restraints;
      std::vector<coot::dict_angle_restraint_t> angle_restraints;
      std::vector<coot::dict_torsion_restraint_t> torsion_restraints;
      std::vector<coot::dict_chiral_restraint_t> chiral_restraints;
      std::vector<coot::dict_plane_restraint_t> plane_restraints;
      std::vector<coot::dict_atom> atoms;
      coot::dict_chem_comp_t residue_info;

      PyObject *key;
      PyObject *value;
      Py_ssize_t pos = 0;

      std::cout << "looping over restraint" << std::endl;
      while (PyDict_Next(restraints, &pos, &key, &value)) {
	 // std::cout << ":::::::key: " << PyString_AsString(key) << std::endl;

	 std::string key_string = PyString_AsString(key);
	 if (key_string == "_chem_comp") {
	    PyObject *chem_comp_list = value;
	    if (PyList_Check(chem_comp_list)) {
	       if (PyObject_Length(chem_comp_list) == 7) {
		  std::string comp_id  = PyString_AsString(PyList_GetItem(chem_comp_list, 0));
		  std::string tlc      = PyString_AsString(PyList_GetItem(chem_comp_list, 1));
		  std::string name     = PyString_AsString(PyList_GetItem(chem_comp_list, 2));
		  std::string group    = PyString_AsString(PyList_GetItem(chem_comp_list, 3));
		  int n_atoms_all      = PyInt_AsLong(PyList_GetItem(chem_comp_list, 4));
		  int n_atoms_nh       = PyInt_AsLong(PyList_GetItem(chem_comp_list, 5));
		  std::string desc_lev = PyString_AsString(PyList_GetItem(chem_comp_list, 6));

		  coot::dict_chem_comp_t n(comp_id, tlc, name, group,
					   n_atoms_all, n_atoms_nh, desc_lev);
		  residue_info = n;
	       }
	    }
	 }


	 if (key_string == "_chem_comp_atom") {
	    PyObject *chem_comp_atom_list = value;
	    if (PyList_Check(chem_comp_atom_list)) {
	       int n_atoms = PyObject_Length(chem_comp_atom_list);
	       for (int iat=0; iat<n_atoms; iat++) {
		  PyObject *chem_comp_atom = PyList_GetItem(chem_comp_atom_list, iat);
		  if (PyObject_Length(chem_comp_atom) == 5) {
		     std::string atom_id  = PyString_AsString(PyList_GetItem(chem_comp_atom, 0));
		     std::string element  = PyString_AsString(PyList_GetItem(chem_comp_atom, 1));
		     std::string energy_t = PyString_AsString(PyList_GetItem(chem_comp_atom, 2));
		     float part_chr        = PyFloat_AsDouble(PyList_GetItem(chem_comp_atom, 3));
		     bool flag = 0;
		     if (PyLong_AsLong(PyList_GetItem(chem_comp_atom, 4))) {
			flag = 1;
		     }
		     std::pair<bool, float> part_charge_info(flag, part_chr);
		     coot::dict_atom at(atom_id, atom_id, element, energy_t, part_charge_info);
		     atoms.push_back(at);
		  }
	       }
	    } 
	 }

	 if (key_string == "_chem_comp_bond") {
	    PyObject *bond_restraint_list = value;
	    if (PyList_Check(bond_restraint_list)) {
	       int n_bonds = PyObject_Length(bond_restraint_list);
	       for (int i_bond=0; i_bond<n_bonds; i_bond++) {
		  PyObject *bond_restraint = PyList_GetItem(bond_restraint_list, i_bond);
		  if (PyObject_Length(bond_restraint) == 4) { 
		     PyObject *atom_1_py = PyList_GetItem(bond_restraint, 0);
		     PyObject *atom_2_py = PyList_GetItem(bond_restraint, 1);
		     PyObject *dist_py   = PyList_GetItem(bond_restraint, 2);
		     PyObject *esd_py    = PyList_GetItem(bond_restraint, 3);

		     if (PyString_Check(atom_1_py) &&
			 PyString_Check(atom_2_py) &&
			 PyFloat_Check(dist_py) && 
			 PyFloat_Check(esd_py)) {
			std::string atom_1 = PyString_AsString(atom_1_py);
			std::string atom_2 = PyString_AsString(atom_2_py);
			float  dist = PyFloat_AsDouble(dist_py);
			float  esd  = PyFloat_AsDouble(esd_py);
			coot::dict_bond_restraint_t rest(atom_1, atom_2, "unknown", dist, esd);
			bond_restraints.push_back(rest);
		     }
		  }
	       }
	    } 
	 }


	 if (key_string == "_chem_comp_angle") {
	    PyObject *angle_restraint_list = value;
	    if (PyList_Check(angle_restraint_list)) {
	       int n_angles = PyObject_Length(angle_restraint_list);
	       for (int i_angle=0; i_angle<n_angles; i_angle++) {
		  PyObject *angle_restraint = PyList_GetItem(angle_restraint_list, i_angle);
		  if (PyObject_Length(angle_restraint) == 5) { 
		     PyObject *atom_1_py = PyList_GetItem(angle_restraint, 0);
		     PyObject *atom_2_py = PyList_GetItem(angle_restraint, 1);
		     PyObject *atom_3_py = PyList_GetItem(angle_restraint, 2);
		     PyObject *angle_py  = PyList_GetItem(angle_restraint, 3);
		     PyObject *esd_py    = PyList_GetItem(angle_restraint, 4);

		     if (PyString_Check(atom_1_py) &&
			 PyString_Check(atom_2_py) &&
			 PyString_Check(atom_3_py) &&
			 PyFloat_Check(angle_py) && 
			 PyFloat_Check(esd_py)) {
			std::string atom_1 = PyString_AsString(atom_1_py);
			std::string atom_2 = PyString_AsString(atom_2_py);
			std::string atom_3 = PyString_AsString(atom_3_py);
			float  angle = PyFloat_AsDouble(angle_py);
			float  esd   = PyFloat_AsDouble(esd_py);
			coot::dict_angle_restraint_t rest(atom_1, atom_2, atom_3, angle, esd);
			angle_restraints.push_back(rest);
		     }
		  }
	       }
	    }
	 }

	 if (key_string == "_chem_comp_tor") {
	    PyObject *torsion_restraint_list = value;
	    if (PyList_Check(torsion_restraint_list)) {
	       int n_torsions = PyObject_Length(torsion_restraint_list);
	       for (int i_torsion=0; i_torsion<n_torsions; i_torsion++) {
		  PyObject *torsion_restraint = PyList_GetItem(torsion_restraint_list, i_torsion);
		  if (PyObject_Length(torsion_restraint) == 7) { 
		     PyObject *atom_1_py = PyList_GetItem(torsion_restraint, 0);
		     PyObject *atom_2_py = PyList_GetItem(torsion_restraint, 1);
		     PyObject *atom_3_py = PyList_GetItem(torsion_restraint, 2);
		     PyObject *atom_4_py = PyList_GetItem(torsion_restraint, 3);
		     PyObject *torsion_py= PyList_GetItem(torsion_restraint, 4);
		     PyObject *esd_py    = PyList_GetItem(torsion_restraint, 5);
		     PyObject *period_py = PyList_GetItem(torsion_restraint, 6);

		     if (PyString_Check(atom_1_py) &&
			 PyString_Check(atom_2_py) &&
			 PyString_Check(atom_3_py) &&
			 PyString_Check(atom_4_py) &&
			 PyFloat_Check(torsion_py) && 
			 PyFloat_Check(esd_py)    && 
			 PyInt_Check(period_py)) { 
			std::string atom_1 = PyString_AsString(atom_1_py);
			std::string atom_2 = PyString_AsString(atom_2_py);
			std::string atom_3 = PyString_AsString(atom_3_py);
			std::string atom_4 = PyString_AsString(atom_4_py);
			float  torsion = PyFloat_AsDouble(torsion_py);
			float  esd     = PyFloat_AsDouble(esd_py);
			int  period    = PyInt_AsLong(period_py);
			coot::dict_torsion_restraint_t rest(atom_1, atom_2, atom_3, atom_4,
							    torsion, esd, period);
			torsion_restraints.push_back(rest);
		     }
		  }
	       }
	    } 
	 }

	 if (key_string == "_chem_comp_chir") {
	    PyObject *chiral_restraint_list = value;
	    if (PyList_Check(chiral_restraint_list)) {
	       int n_chirals = PyObject_Length(chiral_restraint_list);
	       for (int i_chiral=0; i_chiral<n_chirals; i_chiral++) {
		  PyObject *chiral_restraint = PyList_GetItem(chiral_restraint_list, i_chiral);
		  if (PyObject_Length(chiral_restraint) == 7) { 
		     PyObject *chiral_id_py= PyList_GetItem(chiral_restraint, 0);
		     PyObject *atom_c_py   = PyList_GetItem(chiral_restraint, 1);
		     PyObject *atom_1_py   = PyList_GetItem(chiral_restraint, 2);
		     PyObject *atom_2_py   = PyList_GetItem(chiral_restraint, 3);
		     PyObject *atom_3_py   = PyList_GetItem(chiral_restraint, 4);
		     PyObject *vol_sign_py = PyList_GetItem(chiral_restraint, 5);
		     PyObject *esd_py      = PyList_GetItem(chiral_restraint, 6);

		     if (PyString_Check(atom_1_py) &&
			 PyString_Check(atom_2_py) &&
			 PyString_Check(atom_3_py) &&
			 PyString_Check(atom_c_py) &&
			 PyString_Check(chiral_id_py) && 
			 PyFloat_Check(esd_py)    && 
			 PyInt_Check(vol_sign_py)) {
			std::string chiral_id = PyString_AsString(chiral_id_py);
			std::string atom_c    = PyString_AsString(atom_c_py);
			std::string atom_1    = PyString_AsString(atom_1_py);
			std::string atom_2    = PyString_AsString(atom_2_py);
			std::string atom_3    = PyString_AsString(atom_3_py);
			float  esd            = PyFloat_AsDouble(esd_py);
			int  vol_sign         = PyInt_AsLong(vol_sign_py);
			coot::dict_chiral_restraint_t rest(chiral_id,
							   atom_c, atom_1, atom_2, atom_3, 
							   vol_sign);
			chiral_restraints.push_back(rest);
		     }
		  }
	       }
	    } 
	 }


	 if (key_string == "_chem_comp_plane_atom") {
	    PyObject *plane_restraint_list = value;
	    if (PyList_Check(plane_restraint_list)) {
	       int n_planes = PyObject_Length(plane_restraint_list);
	       for (int i_plane=0; i_plane<n_planes; i_plane++) {
		  PyObject *plane_restraint = PyList_GetItem(plane_restraint_list, i_plane);
		  if (PyObject_Length(plane_restraint) == 3) {
		     std::vector<std::string> atoms;
		     PyObject *plane_id_py = PyList_GetItem(plane_restraint, 0);
		     PyObject *esd_py      = PyList_GetItem(plane_restraint, 2);
		     PyObject *py_atoms_py = PyList_GetItem(plane_restraint, 1);

		     bool atoms_pass = 1;
		     if (PyList_Check(py_atoms_py)) {
			int n_atoms = PyObject_Length(py_atoms_py);
			for (int iat=0; iat<n_atoms; iat++) {
			   PyObject *at_py = PyList_GetItem(py_atoms_py, iat);
			   if (PyString_Check(at_py)) {
			      atoms.push_back(PyString_AsString(at_py));
			   } else {
			      atoms_pass = 0;
			   }
			}
			if (atoms_pass) {
			   if (PyString_Check(plane_id_py)) {
			      if (PyFloat_Check(esd_py)) {
				 std::string plane_id = PyString_AsString(plane_id_py);
				 float esd = PyFloat_AsDouble(esd_py);
				 if (atoms.size() > 0) { 
				    coot::dict_plane_restraint_t rest(plane_id, atoms[0], esd);
				    for (int i=1; i<atoms.size(); i++)
				       rest.push_back_atom(atoms[i]);
				    plane_restraints.push_back(rest);
				 }
			      }
			   }
			} 
		     }
		  }
	       }
	    }
	 }
      }
	
      coot::dictionary_residue_restraints_t monomer_restraints(monomer_type, 1);
      monomer_restraints.bond_restraint    = bond_restraints;
      monomer_restraints.angle_restraint   = angle_restraints;
      monomer_restraints.torsion_restraint = torsion_restraints;
      monomer_restraints.chiral_restraint  = chiral_restraints;
      monomer_restraints.plane_restraint   = plane_restraints;
      monomer_restraints.residue_info      = residue_info;
      monomer_restraints.atom_info         = atoms; 
      
      graphics_info_t g;
      bool s = g.Geom_p()->replace_monomer_restraints(monomer_type, monomer_restraints);
      if (s)
	 retval = Py_True;
      
   }
   return retval;
} 
#endif // USE_PYTHON



/*  ----------------------------------------------------------------------- */
/*                  CCP4MG Interface                                        */
/*  ----------------------------------------------------------------------- */
void write_ccp4mg_picture_description(const char *filename) {

   std::ofstream mg_stream(filename);

   if (!mg_stream) {
      std::cout << "WARNING:: can't open file " << filename << std::endl;
   } else {
      mg_stream << "# CCP4mg picture definition file\n";
      mg_stream << "# See http://www.ysbl.york.ac.uk/~ccp4mg/ccp4mg_help/picture_definition.html\n";
      mg_stream << "# or your local copy of CCP4mg documentation.\n";
      mg_stream << "# created by Coot " << coot_version() << "\n";

      // View:
      graphics_info_t g;
      coot::Cartesian rc = g.RotationCentre();
      mg_stream << "view = View (\n";
      mg_stream << "    centre_xyz = [";
      mg_stream << -rc.x() << ", " << -rc.y() << ", " << -rc.z() << "],\n";
      mg_stream << "    radius = " << 0.75*graphics_info_t::zoom << ",\n";
      //       mg_stream << "    orientation = [ " << g.quat[0] << ", "
      // 		<< g.quat[1] << ", " << g.quat[2] << ", " << g.quat[3] << "]\n";
      // Stuart corrects the orientation specification:
      mg_stream << "    orientation = [ " << -g.quat[3] << ", "
		<< g.quat[0] << ", " << g.quat[1] << ", " << g.quat[2] << "]\n";
      mg_stream << ")\n";

      // Molecules:
      for (int imol=0; imol<graphics_info_t::n_molecules(); imol++) {
	 if (is_valid_model_molecule(imol)) {
	    mg_stream << "MolData (\n";
	    mg_stream << "   filename = [\n";
	    mg_stream << "   'FULLPATH',\n";
	    mg_stream << "   " << single_quote(graphics_info_t::molecules[imol].name_) << ",\n";
	    mg_stream << "   " << single_quote(graphics_info_t::molecules[imol].name_) << "])\n";
	    mg_stream << "\n";
	    mg_stream << "MolDisp (\n";
	    mg_stream << "    selection = 'all',\n";
	    mg_stream << "    colour    = 'atomtype',\n"; 
	    mg_stream << "    style     = 'BONDS',\n";
	    mg_stream << "    selection_parameters = {\n";
	    mg_stream << "                'neighb_rad' : '5.0',\n";
	    mg_stream << "                'select' : 'all',\n";
	    mg_stream << "                'neighb_excl' : 0  },\n";
	    mg_stream << "    colour_parameters =  {\n";
	    mg_stream << "                'colour_mode' : 'atomtype',\n";
	    mg_stream << "                'user_scheme' : [ ]  })\n";
	    mg_stream << "\n";
	 }
	 if (is_valid_map_molecule(imol)) {
	    std::string phi = g.molecules[imol].save_phi_col;
	    std::string f   = g.molecules[imol].save_f_col;
	    std::string w   = g.molecules[imol].save_weight_col;
	    float lev       = g.molecules[imol].contour_level[0];
	    float r         = g.box_radius;
	    // int use_weights_flag = g.molecules[imol].save_use_weights;
	    std::string name = single_quote(graphics_info_t::molecules[imol].save_mtz_file_name);
	    mg_stream << "MapData (\n";
	    mg_stream << "   filename = [\n";
	    mg_stream << "   'FULLPATH',\n";
	    mg_stream << "   " << name << ",\n";
	    mg_stream << "   " << name << "],\n";
	    mg_stream << "   phi = " << single_quote(phi) << ",\n";
	    mg_stream << "   f   = " << single_quote(f) << ",\n";
	    mg_stream << "   rate = 0.75\n";
	    mg_stream << ")\n";
	    mg_stream << "MapDisp (\n";
	    mg_stream << "    contour_level = " << lev << ",\n";
	    mg_stream << "    surface_style = 'chickenwire',\n";
	    mg_stream << "    radius = " << r << ",\n";
	    mg_stream << "    clip_mode = 'OFF')\n";
	    mg_stream << "\n";
	 }
      }
   }
} 
