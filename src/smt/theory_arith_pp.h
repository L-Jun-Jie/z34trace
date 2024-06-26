/*++
Copyright (c) 2006 Microsoft Corporation

Module Name:

    theory_arith_pp.h

Abstract:

    <abstract>

Author:

    Leonardo de Moura (leonardo) 2008-05-05.

Revision History:

--*/
#pragma once

#include "smt/theory_arith.h"
#include "ast/ast_smt_pp.h"
#include "util/stats.h"

namespace smt {
    template<typename Ext>
    void theory_arith<Ext>::collect_statistics(::statistics & st) const {
        st.update("arith conflicts", m_stats.m_conflicts);
        st.update("arith row summations", m_stats.m_add_rows);
        st.update("arith num rows", m_rows.size());
        st.update("arith pivots", m_stats.m_pivots);
        st.update("arith assert lower", m_stats.m_assert_lower);
        st.update("arith assert upper", m_stats.m_assert_upper);
        st.update("arith assert diseq", m_stats.m_assert_diseq);
        st.update("arith bound prop", m_stats.m_bound_props);
        st.update("arith fixed eqs", m_stats.m_fixed_eqs);
        st.update("arith assume eqs", m_stats.m_assume_eqs);
        st.update("arith offset eqs", m_stats.m_offset_eqs);
        st.update("arith gcd tests", m_stats.m_gcd_tests);
        st.update("arith gcd conflicts", m_stats.m_gcd_conflicts);
        st.update("arith ineq splits", m_stats.m_branches);
        st.update("arith gomory cuts", m_stats.m_gomory_cuts);
        st.update("arith branch int", m_stats.m_branch_infeasible_int);
        st.update("arith branch var", m_stats.m_branch_infeasible_var);
        st.update("arith patches", m_stats.m_patches);
        st.update("arith patches_succ", m_stats.m_patches_succ);
        st.update("arith max-min", m_stats.m_max_min);
        st.update("arith grobner", m_stats.m_gb_compute_basis);
        st.update("arith pseudo nonlinear", m_stats.m_nl_linear);
        st.update("arith nonlinear bounds", m_stats.m_nl_bounds);
        st.update("arith nonlinear horner", m_stats.m_nl_cross_nested);
        st.update("arith tableau max rows", m_stats.m_tableau_max_rows);
        st.update("arith tableau max columns", m_stats.m_tableau_max_columns);
        m_arith_eq_adapter.collect_statistics(st);
    }

    template<typename Ext>
    void theory_arith<Ext>::display(std::ostream & out) const {
        if (get_num_vars() == 0) return;
        out << "Theory arithmetic:\n";
        display_vars(out);
        display_nl_monomials(out);
        display_rows(out, true);
        display_rows(out, false);
        display_atoms(out);
        display_asserted_atoms(out);
    }

    template<typename Ext>
    void theory_arith<Ext>::display_nl_monomials(std::ostream & out) const {
        if (m_nl_monomials.empty())
            return;
        out << "non linear monomials:\n";
        for (auto nl : m_nl_monomials) 
            display_var(out, nl);
    }

    template<typename Ext>
    void theory_arith<Ext>::display_row(std::ostream & out, unsigned r_id, bool compact) const {
        out << r_id << " ";
        display_row(out, m_rows[r_id], compact);
    }

    template<typename Ext>
    void theory_arith<Ext>::display_row(std::ostream & out, row const & r, bool compact) const {
        if (static_cast<unsigned>(r.get_base_var()) >= m_columns.size())
            return;
        column const & c   = m_columns[r.get_base_var()];
        if (c.size() > 0)
            out << "(v" << r.get_base_var() << " r" << c[0].m_row_id << ") : ";
        bool first = true;
        for (auto const& e : r) {
            if (!e.is_dead()) {
                if (first)
                    first = false;
                else
                    out << " + ";
                theory_var s      = e.m_var;
                numeral const & c = e.m_coeff;
                if (!c.is_one())
                    out << c << "*";
                if (compact) {
                    out << "v" << s;
                    if (is_fixed(s)) {
                        out << ":" << lower(s)->get_value();
                    }
                }
                else
                    out << enode_pp(get_enode(s), ctx);
            }
        }
        out << "\n";
    }


    template<typename Ext>
    void theory_arith<Ext>::display_rows(std::ostream & out, bool compact) const {
        if (compact)
            out << "rows (compact view):\n";
        else
            out << "rows (expanded view):\n";
        unsigned num = m_rows.size();
        for (unsigned r_id = 0; r_id < num; r_id++) 
            if (m_rows[r_id].m_base_var != null_theory_var) 
                display_row(out, r_id, compact);
    }

    template<typename Ext>
    void theory_arith<Ext>::display_row_shape(std::ostream & out, row const & r) const {
        for (auto const& e : r) {
            if (!e.is_dead()) {
                numeral const & c = e.m_coeff;
                if (c.is_one())
                    out << "1";
                else if (c.is_minus_one())
                    out << "-";
                else if (c.is_int() && c.to_rational().is_small()) // small integer
                    out << "i";
                else if (c.is_int() && !c.to_rational().is_small()) // big integer
                    out << "I";
                else if (c.to_rational().is_small()) // small rational
                    out << "r";
                else // big rational
                    out << "R";
            }
        }
        out << "\n";
    }

    template<typename Ext>
    bool theory_arith<Ext>::is_one_minus_one_row(row const & r) const {
        for (auto const& e : r) {
            if (!e.is_dead()) {
                numeral const & c = e.m_coeff;
                if (!c.is_one() && !c.is_minus_one())
                    return false;
            }
        }
        return true;
    }

    template<typename Ext>
    void theory_arith<Ext>::display_rows_shape(std::ostream & out) const {
        unsigned num = m_rows.size();
        unsigned num_trivial = 0; // can be simply solved
        for (unsigned r_id = 0; r_id < num; r_id++) {
            row const & r = m_rows[r_id];
            if (r.m_base_var != null_theory_var) {
                if (is_one_minus_one_row(r)) // all coefficients are unit factors (1 or -1)
                    num_trivial++;
                else
                    display_row_shape(out, r);
            }
        }
        out << "num. trivial: " << num_trivial << "\n";
    }

    template<typename Ext>
    void theory_arith<Ext>::display_rows_bignums(std::ostream & out) const {
        unsigned num = m_rows.size();
        for (unsigned r_id = 0; r_id < num; r_id++) {
            row const & r = m_rows[r_id];
            if (r.m_base_var != null_theory_var) {
                for (auto const& e : r) {
                    if (!e.is_dead()) {
                        numeral const & c = e.m_coeff;
                        if (c.to_rational().is_big()) {
                            std::string str = c.to_rational().to_string();
                            if (str.length() > 48) 
                                out << str << "\n";
                        }
                    }
                }
            }
        }
    }

    template<typename Ext>
    void theory_arith<Ext>::display_rows_stats(std::ostream & out) const {
        unsigned num_vars = get_num_vars();
        unsigned num_rows = 0;
        unsigned num_non_zeros  = 0;
        unsigned num_ones       = 0;
        unsigned num_minus_ones = 0;
        unsigned num_small_ints = 0;
        unsigned num_big_ints   = 0;
        unsigned num_small_rats = 0;
        unsigned num_big_rats   = 0;
        for (unsigned r_id = 0; r_id < m_rows.size(); r_id++) { // iterate over all rows
            row const & r = m_rows[r_id];
            if (r.m_base_var != null_theory_var) {
                num_rows++;
                for (auto const& e : r) {
                    if (!e.is_dead()) {
                        numeral const & c = e.m_coeff;
                        num_non_zeros++;
                        if (c.is_one())
                            num_ones++;
                        else if (c.is_minus_one())
                            num_minus_ones++;
                        else if (c.is_int() && c.to_rational().is_small())
                            num_small_ints++;
                        else if (c.is_int() && !c.to_rational().is_small())
                            num_big_ints++;
                        else if (c.to_rational().is_small())
                            num_small_rats++;
                        else
                            num_big_rats++;
                    }
                }
            }
        }
        out << "A:        " << num_rows << " X " << num_vars << "\n";
        out << "avg. row: " << num_non_zeros / num_rows << ", num. non zeros: " << num_non_zeros << "\n";
        unsigned spc = 6;
        out.width(spc);
        out << 1 << "|";
        out.width(spc);
        out << -1 << "|";
        out.width(spc);
        out << "i";
        out << "|";
        out.width(spc);
        out << "I";
        out << "|";
        out.width(spc);
        out << "r";
        out << "|";
        out.width(spc);
        out << "R";
        out << "\n";

        out.width(spc);
        out << num_ones << "|";
        out.width(spc);
        out << num_minus_ones << "|";
        out.width(spc);
        out << num_small_ints;
        out << "|";
        out.width(spc);
        out << num_big_ints;
        out << "|";
        out.width(spc);
        out << num_small_rats;
        out << "|";
        out.width(spc);
        out << num_big_rats;
        out << "\n";
    }

    template<typename Ext>
    void theory_arith<Ext>::display_row_info(std::ostream & out, unsigned r_id) const {
        out << r_id << " ";
        display_row_info(out, m_rows[r_id]);
    }

    template<typename Ext>
    void theory_arith<Ext>::display_row_info(std::ostream & out, row const & r) const {
        display_row(out, r, true);
        for (auto const& e : r) 
            if (!e.is_dead())
                display_var(out, e.m_var);
    }

    /**
       \brief Display row after substituting fixed variables.
    */
    template<typename Ext>
    void theory_arith<Ext>::display_simplified_row(std::ostream & out, row const & r) const {
        bool has_rat_coeff = false;
        numeral k;
        
        out << "(v" << r.get_base_var() << ") : ";
        bool first = true;
        for (auto const& e : r) {
            if (e.is_dead())
                continue;
            theory_var v      = e.m_var;
            numeral const & c = e.m_coeff;
            if (is_fixed(v)) {
                k += c * lower_bound(v).get_rational(); 
                continue;
            }
            if (!c.is_int())
                has_rat_coeff = true;
            if (first)
                first = false;
            else
                out << " + ";
            if (!c.is_one())
                out << c << "*";
            out << "v" << v;
        }
        if (!k.is_zero()) {
            if (!first)
                out << " + ";
            out << k;
        }
        out << "\n";
        if (has_rat_coeff) {
            for (auto const& e : r) 
                if (!e.is_dead() && (is_base(e.m_var) || (!is_fixed(e.m_var) && (lower(e.m_var) || upper(e.m_var)))))
                    display_var(out, e.m_var);
        }
    }

    template<typename Ext>
    void theory_arith<Ext>::display_var(std::ostream & out, theory_var v) const {
        out << "v";
        out.width(4);
        out << std::left << v;
        out << " #";
        out.width(4);
        out << get_enode(v)->get_owner_id();
        out << std::right;
        out << " lo:";
        out.width(10);
        if (lower(v)) {
            out << lower(v)->get_value();
        }
        else {
            out << "-oo";
        }
        out << ", up:";
        out.width(10);
        if (upper(v)) {
            out << upper(v)->get_value();
        }
        else {
            out << "oo";
        }
        out << ", value: ";
        out.width(10);
        out << get_value(v);
        out << ", occs: ";
        out.width(4);
        out << m_columns[v].size();
        out << ", atoms: ";
        out.width(4);
        out << m_var_occs[v].size();
        out << (is_int(v) ? ", int " : ", real");
        switch (get_var_kind(v)) {
        case NON_BASE:
            out << ", non-base  ";
            break;
        case QUASI_BASE:
            out << ", quasi-base";
            break;
        case BASE:
            out << ", base      ";
            break;
        }
        out << ", shared: " << get_context().is_shared(get_enode(v));
        out << ", unassigned: " << m_unassigned_atoms[v];
        out << ", rel: " << get_context().is_relevant(get_enode(v));
        out << ", def: " << enode_pp(get_enode(v), ctx);
        out << "\n";
    }

    template<typename Ext>
    void theory_arith<Ext>::display_vars(std::ostream & out) const {
        out << "vars:\n";
        int n = get_num_vars();
        int inf_vars = 0;
        int int_inf_vars = 0;
        for (theory_var v = 0; v < n; v++) {
            if ((lower(v) && lower(v)->get_value() > get_value(v))
                || (upper(v) && upper(v)->get_value() < get_value(v)))
                inf_vars++;
            if (is_int(v) && !get_value(v).is_int())
                int_inf_vars++;
        }
        out << "infeasibles = " << inf_vars << " int_inf = " << int_inf_vars << std::endl;
        for (theory_var v = 0; v < n; v++) {
            display_var(out, v);
        }
    }

    template<typename Ext>
    void theory_arith<Ext>::display_bound(std::ostream & out, bound * b, unsigned indent) const {
        for (unsigned i = 0; i < indent; i++) out << "  ";
        b->display(*this, out);
        out << "\n";
    }

    template<typename Ext>
    std::ostream& theory_arith<Ext>::antecedents_t::display(theory_arith& th, std::ostream & out) const {
        th.get_context().display_literals_verbose(out, lits().size(), lits().data());
        if (!lits().empty()) out << "\n";
        ast_manager& m = th.get_manager();
        for (auto const& e : m_eqs) {
            out << mk_pp(e.first->get_expr(), m) << " ";
            out << mk_pp(e.second->get_expr(), m) << "\n";            
        }
        return out;
    }

    template<typename Ext>
    void theory_arith<Ext>::display_deps(std::ostream & out, v_dependency* dep) {
        ptr_vector<void> bounds;
        m_dep_manager.linearize(dep, bounds);
        m_tmp_lit_set.reset();
        m_tmp_eq_set.reset();
        for (void *_b : bounds) {
            bound * b = static_cast<bound*>(_b);
            b->display(*this, out << "\n");
        }
    }

    template<typename Ext>
    void theory_arith<Ext>::display_interval(std::ostream & out, interval const& i) {
        i.display(out);
        display_deps(out << "\nlo:", i.get_lower_dependencies());
        display_deps(out << "\nhi:", i.get_upper_dependencies());
    }

    template<typename Ext>
    void theory_arith<Ext>::display_atoms(std::ostream & out) const {
        out << "atoms:\n";
        for (atom * a : m_atoms) 
            display_atom(out, a, false);
    }

    template<typename Ext>
    void theory_arith<Ext>::display_asserted_atoms(std::ostream & out) const {
        out << "asserted atoms:\n";
        for (unsigned i = 0; i < m_asserted_qhead; i++) {
            bound * b = m_asserted_bounds[i];
            if (b->is_atom()) 
                display_atom(out, static_cast<atom*>(b), true);
        }
        if (m_asserted_qhead < m_asserted_bounds.size()) {
            out << "delayed atoms:\n";
            for (unsigned i = m_asserted_qhead; i < m_asserted_bounds.size(); i++) {
                bound * b = m_asserted_bounds[i];
                if (b->is_atom()) 
                    display_atom(out, static_cast<atom*>(b), true);
            }
        }
    }

    template<typename Ext>
    void theory_arith<Ext>::display_atom(std::ostream & out, atom * a, bool show_sign) const {
        theory_var      v = a->get_var();
        inf_numeral const & k = a->get_k();
        enode *         e = get_enode(v);
        if (show_sign)
            out << (a->is_true()?"    ":"not ");
        out << "v";
        out.width(3);
        out << std::left << v << " #";
        out.width(3);
        out << e->get_owner_id();
        out << std::right;
        out << " " << ((a->get_atom_kind() == A_LOWER)? ">=" : "<=") << " ";
        out.width(6);
        out << k << "    " << enode_pp(get_enode(v), ctx) << "\n";
    }

    template<typename Ext>
    void theory_arith<Ext>::display_bounds_in_smtlib(std::ostream & out) const {
        ast_manager & m = get_manager();
        ast_smt_pp pp(m);
        pp.set_benchmark_name("lemma");
        int n = get_num_vars();
        for (theory_var v = 0; v < n; v++) {
            expr * n = get_enode(v)->get_expr();
            if (is_fixed(v)) {
                inf_numeral k_inf = lower_bound(v);
                rational k = k_inf.get_rational().to_rational();
                expr_ref eq(m);
                eq = m.mk_eq(n, m_util.mk_numeral(k, is_int(v)));
                pp.add_assumption(eq);
            }
            else {
                if (lower(v) != nullptr) {
                    inf_numeral k_inf = lower_bound(v);
                    rational k = k_inf.get_rational().to_rational();
                    expr_ref ineq(m);
                    if (k_inf.get_infinitesimal().is_zero())
                        ineq = m_util.mk_le(m_util.mk_numeral(k, is_int(v)), n);
                    else
                        ineq = m_util.mk_lt(m_util.mk_numeral(k, is_int(v)), n);
                    pp.add_assumption(ineq);
                }
                if (upper(v) != nullptr) {
                    inf_numeral k_inf = upper_bound(v);
                    rational k = k_inf.get_rational().to_rational();
                    expr_ref ineq(m);
                    if (k_inf.get_infinitesimal().is_zero())
                        ineq = m_util.mk_le(n, m_util.mk_numeral(k, is_int(v)));
                    else
                        ineq = m_util.mk_lt(n, m_util.mk_numeral(k, is_int(v)));
                    pp.add_assumption(ineq);
                }
            }
        }
        pp.display_smt2(out, m.mk_true());
    }

    template<typename Ext>
    void theory_arith<Ext>::display_bounds_in_smtlib() const {
        static int id = 0;
        std::string buffer = "arith_" + std::to_string(id) + ".smt2";
        std::ofstream out(buffer);
        display_bounds_in_smtlib(out);
        out.close();
        id++;
    }

};


