/*
 *	Filters: Instructions themselves
 *
 *	Copyright 1998 Pavel Machek <pavel@ucw.cz>
 *	Copyright 2018 Maria Matejka <mq@jmq.cz>
 *	Copyright 2018 CZ.NIC z.s.p.o.
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 *
 */

/* Binary operators */
  case FI_ADD:
    ARG(1,T_INT);
    ARG(2,T_INT);
    res.type = T_INT;
    res.val.i = v1.val.i + v2.val.i;
    break;
  case FI_SUBTRACT:
    ARG(1,T_INT);
    ARG(2,T_INT);
    res.type = T_INT;
    res.val.i = v1.val.i - v2.val.i;
    break;
  case FI_MULTIPLY:
    ARG(1,T_INT);
    ARG(2,T_INT);
    res.type = T_INT;
    res.val.i = v1.val.i * v2.val.i;
    break;
  case FI_DIVIDE:
    ARG(1,T_INT);
    ARG(2,T_INT);
    res.type = T_INT;
    if (v2.val.i == 0) runtime( "Mother told me not to divide by 0" );
    res.val.i = v1.val.i / v2.val.i;
    break;
  case FI_AND:
  case FI_OR:
    ARG(1,T_BOOL);
    if (v1.val.i == (what->fi_code == FI_OR)) {
      res.type = T_BOOL;
      res.val.i = v1.val.i;
    } else {
      ARG(2,T_BOOL);
      res = v2;
    }
    break;
  case FI_PAIR_CONSTRUCT:
    ARG(1,T_INT);
    ARG(2,T_INT);
    u1 = v1.val.i;
    u2 = v2.val.i;
    if ((u1 > 0xFFFF) || (u2 > 0xFFFF))
      runtime( "Can't operate with value out of bounds in pair constructor" );
    res.val.i = (u1 << 16) | u2;
    res.type = T_PAIR;
    break;

  case FI_EC_CONSTRUCT:
    {
      ARG_ANY(1);
      ARG(2, T_INT);

      int check, ipv4_used;
      u32 key, val;

      if (v1.type == T_INT) {
	ipv4_used = 0; key = v1.val.i;
      }
      else if (v1.type == T_QUAD) {
	ipv4_used = 1; key = v1.val.i;
      }
      /* IP->Quad implicit conversion */
      else if (val_is_ip4(v1)) {
	ipv4_used = 1; key = ipa_to_u32(v1.val.ip);
      }
      else
	runtime("Can't operate with key of non-integer/IPv4 type in EC constructor");

      val = v2.val.i;

      /* XXXX */
      res.type = T_EC;

      if (what->aux == EC_GENERIC) {
	check = 0; res.val.ec = ec_generic(key, val);
      }
      else if (ipv4_used) {
	check = 1; res.val.ec = ec_ip4(what->aux, key, val);
      }
      else if (key < 0x10000) {
	check = 0; res.val.ec = ec_as2(what->aux, key, val);
      }
      else {
	check = 1; res.val.ec = ec_as4(what->aux, key, val);
      }

      if (check && (val > 0xFFFF))
	runtime("Can't operate with value out of bounds in EC constructor");

      break;
    }

  case FI_LC_CONSTRUCT:
    {
      ARG(1, T_INT);
      ARG(2, T_INT);
      ARG(3, T_INT);

      res.type = T_LC;
      res.val.lc = (lcomm) { v1.val.i, v2.val.i, v3.val.i };

      break;
    }

  case FI_PATHMASK_CONSTRUCT:
    {
      struct f_path_mask *tt = what->a1.p, *vbegin, **vv = &vbegin;

      while (tt) {
	*vv = lp_alloc(fs->pool, sizeof(struct f_path_mask));
	if (tt->kind == PM_ASN_EXPR) {
	  struct f_val xres;
	  INTERPRET(xres, (struct f_inst *) tt->val);
	  (*vv)->kind = PM_ASN;
	  if (xres.type != T_INT) {
	    runtime( "Error resolving path mask template: value not an integer" );
	    return F_ERROR;
	  }

	  (*vv)->val = xres.val.i;
	} else {
	  **vv = *tt;
	}
	tt = tt->next;
	vv = &((*vv)->next);
      }

      res = (struct f_val) { .type = T_PATH_MASK, .val.path_mask = vbegin };
      break;
    }

/* Relational operators */

#define COMPARE(x) \
    ARG_ANY(1); \
    ARG_ANY(2); \
    i = val_compare(v1, v2); \
    if (i==CMP_ERROR) \
      runtime( "Can't compare values of incompatible types" ); \
    res.type = T_BOOL; \
    res.val.i = (x); \
    break;

#define SAME(x) \
    ARG_ANY(1); \
    ARG_ANY(2); \
    i = val_same(v1, v2); \
    res.type = T_BOOL; \
    res.val.i = (x); \
    break;

  case FI_NEQ: SAME(!i);
  case FI_EQ: SAME(i);
  case FI_LT: COMPARE(i==-1);
  case FI_LTE: COMPARE(i!=1);

  case FI_NOT:
    ARG(1,T_BOOL);
    res = v1;
    res.val.i = !res.val.i;
    break;

  case FI_MATCH:
    ARG_ANY(1);
    ARG_ANY(2);
    res.type = T_BOOL;
    res.val.i = val_in_range(v1, v2);
    if (res.val.i == CMP_ERROR)
      runtime( "~ applied on unknown type pair" );
    res.val.i = !!res.val.i;
    break;

  case FI_NOT_MATCH:
    ARG_ANY(1);
    ARG_ANY(2);
    res.type = T_BOOL;
    res.val.i = val_in_range(v1, v2);
    if (res.val.i == CMP_ERROR)
      runtime( "!~ applied on unknown type pair" );
    res.val.i = !res.val.i;
    break;

  case FI_DEFINED:
    ARG_ANY(1);
    res.type = T_BOOL;
    res.val.i = (v1.type != T_VOID) && !undef_value(v1);
    break;
  case FI_TYPE:
    ARG_ANY(1); /* There may be more types supporting this operation */
    switch (v1.type)
    {
      case T_NET:
	res.type = T_ENUM_NETTYPE;
	res.val.i = v1.val.net->type;
	break;
      default:
	runtime( "Can't determine type of this item" );
    }
    break;
  case FI_IS_V4:
    ARG(1, T_IP);
    res.type = T_BOOL;
    res.val.i = ipa_is_ip4(v1.val.ip);
    break;

  /* Set to indirect value, a1 = variable, a2 = value */
  case FI_SET:
    ARG_ANY(2);
    sym = what->a1.p;
    vp = sym->def;
    if ((sym->class != (SYM_VARIABLE | v2.type)) && (v2.type != T_VOID))
    {
      /* IP->Quad implicit conversion */
      if ((sym->class == (SYM_VARIABLE | T_QUAD)) && val_is_ip4(v2))
      {
	vp->type = T_QUAD;
	vp->val.i = ipa_to_u32(v2.val.ip);
	break;
      }
      runtime( "Assigning to variable of incompatible type" );
    }
    *vp = v2;
    break;

    /* some constants have value in a2, some in *a1.p, strange. */
  case FI_CONSTANT:	/* integer (or simple type) constant, string, set, or prefix_set */
    res.type = what->aux;

    if (res.type == T_PREFIX_SET)
      res.val.ti = what->a2.p;
    else if (res.type == T_SET)
      res.val.t = what->a2.p;
    else if (res.type == T_STRING)
      res.val.s = what->a2.p;
    else
      res.val.i = what->a2.i;
    break;
  case FI_VARIABLE:
  case FI_CONSTANT_INDIRECT:
    res = * ((struct f_val *) what->a1.p);
    break;
  case FI_PRINT:
    ARG_ANY(1);
    val_format(v1, &fs->buf);
    break;
  case FI_CONDITION:	/* ? has really strange error value, so we can implement if ... else nicely :-) */
    ARG(1, T_BOOL);
    if (v1.val.i) {
      ARG_ANY(2);
      res.val.i = 0;
    } else
      res.val.i = 1;
    res.type = T_BOOL;
    break;
  case FI_NOP:
    debug( "No operation\n" );
    break;
  case FI_PRINT_AND_DIE:
    ARG_ANY(1);
    if ((what->a2.i == F_NOP || (what->a2.i != F_NONL && what->a1.p)) &&
	!(fs->flags & FF_SILENT))
      log_commit(*L_INFO, &fs->buf);

    switch (what->a2.i) {
    case F_QUITBIRD:
      die( "Filter asked me to die" );
    case F_ACCEPT:
      /* Should take care about turning ACCEPT into MODIFY */
    case F_ERROR:
    case F_REJECT:	/* FIXME (noncritical) Should print complete route along with reason to reject route */
      return what->a2.i;	/* We have to return now, no more processing. */
    case F_NONL:
    case F_NOP:
      break;
    default:
      bug( "unknown return type: Can't happen");
    }
    break;
  case FI_RTA_GET:	/* rta access */
    {
      ACCESS_RTE;
      struct rta *rta = (*fs->rte)->attrs;
      res.type = what->aux;

      switch (what->a2.i)
      {
      case SA_FROM:	res.val.ip = rta->from; break;
      case SA_GW:	res.val.ip = rta->nh.gw; break;
      case SA_NET:	res.val.net = (*fs->rte)->netA; break;
      case SA_PROTO:	res.val.s = rta->src->proto->name; break;
      case SA_SOURCE:	res.val.i = rta->source; break;
      case SA_SCOPE:	res.val.i = rta->scope; break;
      case SA_DEST:	res.val.i = rta->dest; break;
      case SA_IFNAME:	res.val.s = rta->nh.iface ? rta->nh.iface->name : ""; break;
      case SA_IFINDEX:	res.val.i = rta->nh.iface ? rta->nh.iface->index : 0; break;

      default:
	bug("Invalid static attribute access (%x)", res.type);
      }
    }
    break;
  case FI_RTA_SET:
    ACCESS_RTE;
    ARG_ANY(1);
    if (what->aux != v1.type)
      runtime( "Attempt to set static attribute to incompatible type" );

    f_rta_cow(fs);
    {
      struct rta *rta = (*fs->rte)->attrs;

      switch (what->a2.i)
      {
      case SA_FROM:
	rta->from = v1.val.ip;
	break;

      case SA_GW:
	{
	  ip_addr ip = v1.val.ip;
	  neighbor *n = neigh_find(rta->src->proto, ip, NULL, 0);
	  if (!n || (n->scope == SCOPE_HOST))
	    runtime( "Invalid gw address" );

	  rta->dest = RTD_UNICAST;
	  rta->nh.gw = ip;
	  rta->nh.iface = n->iface;
	  rta->nh.next = NULL;
	  rta->hostentry = NULL;
	}
	break;

      case SA_SCOPE:
	rta->scope = v1.val.i;
	break;

      case SA_DEST:
	i = v1.val.i;
	if ((i != RTD_BLACKHOLE) && (i != RTD_UNREACHABLE) && (i != RTD_PROHIBIT))
	  runtime( "Destination can be changed only to blackhole, unreachable or prohibit" );

	rta->dest = i;
	rta->nh.gw = IPA_NONE;
	rta->nh.iface = NULL;
	rta->nh.next = NULL;
	rta->hostentry = NULL;
	break;

      case SA_IFNAME:
	{
	  struct iface *ifa = if_find_by_name(v1.val.s);
	  if (!ifa)
	    runtime( "Invalid iface name" );

	  rta->dest = RTD_UNICAST;
	  rta->nh.gw = IPA_NONE;
	  rta->nh.iface = ifa;
	  rta->nh.next = NULL;
	  rta->hostentry = NULL;
	}
	break;

      default:
	bug("Invalid static attribute access (%x)", res.type);
      }
    }
    break;
  case FI_EA_GET:	/* Access to extended attributes */
    ACCESS_RTE;
    ACCESS_EATTRS;
    {
      u16 code = what->a2.i;
      int f_type = what->aux >> 8;
      eattr *e = ea_find(*fs->eattrs, code);

      if (!e) {
	/* A special case: undefined as_path looks like empty as_path */
	if ((what->aux & EAF_TYPE_MASK) == EAF_TYPE_AS_PATH) {
	  res.type = T_PATH;
	  res.val.ad = &undef_adata;
	  break;
	}

	/* The same special case for int_set */
	if ((what->aux & EAF_TYPE_MASK) == EAF_TYPE_INT_SET) {
	  res.type = T_CLIST;
	  res.val.ad = &undef_adata;
	  break;
	}

	/* The same special case for ec_set */
	if ((what->aux & EAF_TYPE_MASK) == EAF_TYPE_EC_SET) {
	  res.type = T_ECLIST;
	  res.val.ad = &undef_adata;
	  break;
	}

	/* The same special case for lc_set */
	if ((what->aux & EAF_TYPE_MASK) == EAF_TYPE_LC_SET) {
	  res.type = T_LCLIST;
	  res.val.ad = &undef_adata;
	  break;
	}

	/* Undefined value */
	res.type = T_VOID;
	break;
      }

      switch (e->type & EAF_TYPE_MASK) {
      case EAF_TYPE_INT:
	res.type = f_type;
	res.val.i = e->u.data;
	break;
      case EAF_TYPE_ROUTER_ID:
	res.type = T_QUAD;
	res.val.i = e->u.data;
	break;
      case EAF_TYPE_OPAQUE:
	res.type = T_ENUM_EMPTY;
	res.val.i = 0;
	break;
      case EAF_TYPE_IP_ADDRESS:
	res.type = T_IP;
	struct adata * ad = e->u.ptr;
	res.val.ip = * (ip_addr *) ad->data;
	break;
      case EAF_TYPE_AS_PATH:
        res.type = T_PATH;
	res.val.ad = e->u.ptr;
	break;
      case EAF_TYPE_BITFIELD:
	res.type = T_BOOL;
	res.val.i = !!(e->u.data & BITFIELD_MASK(what));
	break;
      case EAF_TYPE_INT_SET:
	res.type = T_CLIST;
	res.val.ad = e->u.ptr;
	break;
      case EAF_TYPE_EC_SET:
	res.type = T_ECLIST;
	res.val.ad = e->u.ptr;
	break;
      case EAF_TYPE_LC_SET:
	res.type = T_LCLIST;
	res.val.ad = e->u.ptr;
	break;
      case EAF_TYPE_UNDEF:
	res.type = T_VOID;
	break;
      default:
	bug("Unknown type in e,a");
      }
    }
    break;
  case FI_EA_SET:
    ACCESS_RTE;
    ACCESS_EATTRS;
    ARG_ANY(1);
    {
      struct ea_list *l = lp_alloc(fs->pool, sizeof(struct ea_list) + sizeof(eattr));
      u16 code = what->a2.i;
      int f_type = what->aux >> 8;

      l->next = NULL;
      l->flags = EALF_SORTED;
      l->count = 1;
      l->attrs[0].id = code;
      l->attrs[0].flags = 0;
      l->attrs[0].type = (what->aux & 0xff) | EAF_ORIGINATED | EAF_FRESH;

      switch (what->aux & EAF_TYPE_MASK) {
      case EAF_TYPE_INT:
	if (v1.type != f_type)
	  runtime( "Setting int attribute to non-int value" );
	l->attrs[0].u.data = v1.val.i;
	break;

      case EAF_TYPE_ROUTER_ID:
	/* IP->Quad implicit conversion */
	if (val_is_ip4(v1)) {
	  l->attrs[0].u.data = ipa_to_u32(v1.val.ip);
	  break;
	}
	/* T_INT for backward compatibility */
	if ((v1.type != T_QUAD) && (v1.type != T_INT))
	  runtime( "Setting quad attribute to non-quad value" );
	l->attrs[0].u.data = v1.val.i;
	break;

      case EAF_TYPE_OPAQUE:
	runtime( "Setting opaque attribute is not allowed" );
	break;
      case EAF_TYPE_IP_ADDRESS:
	if (v1.type != T_IP)
	  runtime( "Setting ip attribute to non-ip value" );
	int len = sizeof(ip_addr);
	struct adata *ad = lp_alloc(fs->pool, sizeof(struct adata) + len);
	ad->length = len;
	(* (ip_addr *) ad->data) = v1.val.ip;
	l->attrs[0].u.ptr = ad;
	break;
      case EAF_TYPE_AS_PATH:
	if (v1.type != T_PATH)
	  runtime( "Setting path attribute to non-path value" );
	l->attrs[0].u.ptr = v1.val.ad;
	break;
      case EAF_TYPE_BITFIELD:
	if (v1.type != T_BOOL)
	  runtime( "Setting bit in bitfield attribute to non-bool value" );
	{
	  /* First, we have to find the old value */
	  eattr *e = ea_find(*fs->eattrs, code);
	  u32 data = e ? e->u.data : 0;

	  if (v1.val.i)
	    l->attrs[0].u.data = data | BITFIELD_MASK(what);
	  else
	    l->attrs[0].u.data = data & ~BITFIELD_MASK(what);;
	}
	break;
      case EAF_TYPE_INT_SET:
	if (v1.type != T_CLIST)
	  runtime( "Setting clist attribute to non-clist value" );
	l->attrs[0].u.ptr = v1.val.ad;
	break;
      case EAF_TYPE_EC_SET:
	if (v1.type != T_ECLIST)
	  runtime( "Setting eclist attribute to non-eclist value" );
	l->attrs[0].u.ptr = v1.val.ad;
	break;
      case EAF_TYPE_LC_SET:
	if (v1.type != T_LCLIST)
	  runtime( "Setting lclist attribute to non-lclist value" );
	l->attrs[0].u.ptr = v1.val.ad;
	break;
      case EAF_TYPE_UNDEF:
	if (v1.type != T_VOID)
	  runtime( "Setting void attribute to non-void value" );
	l->attrs[0].u.data = 0;
	break;
      default: bug("Unknown type in e,S");
      }

      f_rta_cow(fs);
      l->next = *fs->eattrs;
      *fs->eattrs = l;
    }
    break;
  case FI_PREF_GET:
    ACCESS_RTE;
    res.type = T_INT;
    res.val.i = (*fs->rte)->pref;
    break;
  case FI_PREF_SET:
    ACCESS_RTE;
    ARG(1,T_INT);
    if (v1.val.i > 0xFFFF)
      runtime( "Setting preference value out of bounds" );
    f_rte_cow(fs);
    (*fs->rte)->pref = v1.val.i;
    break;
  case FI_LENGTH:	/* Get length of */
    ARG_ANY(1);
    res.type = T_INT;
    switch(v1.type) {
    case T_NET:    res.val.i = net_pxlen(v1.val.net); break;
    case T_PATH:   res.val.i = as_path_getlen(v1.val.ad); break;
    case T_CLIST:  res.val.i = int_set_get_size(v1.val.ad); break;
    case T_ECLIST: res.val.i = ec_set_get_size(v1.val.ad); break;
    case T_LCLIST: res.val.i = lc_set_get_size(v1.val.ad); break;
    default: runtime( "Prefix, path, clist or eclist expected" );
    }
    break;
  case FI_SADR_SRC: 	/* Get SADR src prefix */
    ARG(1, T_NET);
    if (!net_is_sadr(v1.val.net))
      runtime( "SADR expected" );

    {
      net_addr_ip6_sadr *net = (void *) v1.val.net;
      net_addr *src = lp_alloc(fs->pool, sizeof(net_addr_ip6));
      net_fill_ip6(src, net->src_prefix, net->src_pxlen);

      res.type = T_NET;
      res.val.net = src;
    }
    break;
  case FI_ROA_MAXLEN: 	/* Get ROA max prefix length */
    ARG(1, T_NET);
    if (!net_is_roa(v1.val.net))
      runtime( "ROA expected" );

    res.type = T_INT;
    res.val.i = (v1.val.net->type == NET_ROA4) ?
      ((net_addr_roa4 *) v1.val.net)->max_pxlen :
      ((net_addr_roa6 *) v1.val.net)->max_pxlen;
    break;
  case FI_ROA_ASN: 	/* Get ROA ASN */
    ARG(1, T_NET);
    if (!net_is_roa(v1.val.net))
      runtime( "ROA expected" );

    res.type = T_INT;
    res.val.i = (v1.val.net->type == NET_ROA4) ?
      ((net_addr_roa4 *) v1.val.net)->asn :
      ((net_addr_roa6 *) v1.val.net)->asn;
    break;
  case FI_IP:	/* Convert prefix to ... */
    ARG(1, T_NET);
    res.type = T_IP;
    res.val.ip = net_prefix(v1.val.net);
    break;
  case FI_ROUTE_DISTINGUISHER:
    ARG(1, T_NET);
    res.type = T_IP;
    if (!net_is_vpn(v1.val.net))
      runtime( "VPN address expected" );
    res.type = T_RD;
    res.val.ec = net_rd(v1.val.net);
    break;
  case FI_AS_PATH_FIRST:	/* Get first ASN from AS PATH */
    ARG(1, T_PATH);

    as = 0;
    as_path_get_first(v1.val.ad, &as);
    res.type = T_INT;
    res.val.i = as;
    break;
  case FI_AS_PATH_LAST:	/* Get last ASN from AS PATH */
    ARG(1, T_PATH);

    as = 0;
    as_path_get_last(v1.val.ad, &as);
    res.type = T_INT;
    res.val.i = as;
    break;
  case FI_AS_PATH_LAST_NAG:	/* Get last ASN from non-aggregated part of AS PATH */
    ARG(1, T_PATH);

    res.type = T_INT;
    res.val.i = as_path_get_last_nonaggregated(v1.val.ad);
    break;
  case FI_RETURN:
    ARG_ANY(1);
    res = v1;
    return F_RETURN;
  case FI_CALL:
    ARG_ANY(1);
    fret = interpret(fs, what->a2.p, &res);
    if (fret > F_RETURN)
      return fret;
    break;
  case FI_CLEAR_LOCAL_VARS:	/* Clear local variables */
    for (sym = what->a1.p; sym != NULL; sym = sym->aux2)
      ((struct f_val *) sym->def)->type = T_VOID;
    break;
  case FI_SWITCH:
    ARG_ANY(1);
    {
      struct f_tree *t = find_tree(what->a2.p, v1);
      if (!t) {
	v1.type = T_VOID;
	t = find_tree(what->a2.p, v1);
	if (!t) {
	  debug( "No else statement?\n");
	  break;
	}
      }
      /* It is actually possible to have t->data NULL */

      INTERPRET(res, t->data);
    }
    break;
  case FI_IP_MASK: /* IP.MASK(val) */
    ARG(1, T_IP);
    ARG(2, T_INT);

    res.type = T_IP;
    res.val.ip = ipa_is_ip4(v1.val.ip) ?
      ipa_from_ip4(ip4_and(ipa_to_ip4(v1.val.ip), ip4_mkmask(v2.val.i))) :
      ipa_from_ip6(ip6_and(ipa_to_ip6(v1.val.ip), ip6_mkmask(v2.val.i)));
    break;

  case FI_EMPTY:	/* Create empty attribute */
    res.type = what->aux;
    res.val.ad = adata_empty(fs->pool, 0);
    break;
  case FI_PATH_PREPEND:	/* Path prepend */
    ARG(1, T_PATH);
    ARG(2, T_INT);

    res.type = T_PATH;
    res.val.ad = as_path_prepend(fs->pool, v1.val.ad, v2.val.i);
    break;

  case FI_CLIST_ADD_DEL:	/* (Extended) Community list add or delete */
    ARG_ANY(1);
    ARG_ANY(2);
    if (v1.type == T_PATH)
    {
      struct f_tree *set = NULL;
      u32 key = 0;
      int pos;

      if (v2.type == T_INT)
	key = v2.val.i;
      else if ((v2.type == T_SET) && (v2.val.t->from.type == T_INT))
	set = v2.val.t;
      else
	runtime("Can't delete non-integer (set)");

      switch (what->aux)
      {
      case 'a':	runtime("Can't add to path");
      case 'd':	pos = 0; break;
      case 'f':	pos = 1; break;
      default:	bug("unknown Ca operation");
      }

      if (pos && !set)
	runtime("Can't filter integer");

      res.type = T_PATH;
      res.val.ad = as_path_filter(fs->pool, v1.val.ad, set, key, pos);
    }
    else if (v1.type == T_CLIST)
    {
      /* Community (or cluster) list */
      struct f_val dummy;
      int arg_set = 0;
      uint n = 0;

      if ((v2.type == T_PAIR) || (v2.type == T_QUAD))
	n = v2.val.i;
      /* IP->Quad implicit conversion */
      else if (val_is_ip4(v2))
	n = ipa_to_u32(v2.val.ip);
      else if ((v2.type == T_SET) && clist_set_type(v2.val.t, &dummy))
	arg_set = 1;
      else if (v2.type == T_CLIST)
	arg_set = 2;
      else
	runtime("Can't add/delete non-pair");

      res.type = T_CLIST;
      switch (what->aux)
      {
      case 'a':
	if (arg_set == 1)
	  runtime("Can't add set");
	else if (!arg_set)
	  res.val.ad = int_set_add(fs->pool, v1.val.ad, n);
	else
	  res.val.ad = int_set_union(fs->pool, v1.val.ad, v2.val.ad);
	break;

      case 'd':
	if (!arg_set)
	  res.val.ad = int_set_del(fs->pool, v1.val.ad, n);
	else
	  res.val.ad = clist_filter(fs->pool, v1.val.ad, v2, 0);
	break;

      case 'f':
	if (!arg_set)
	  runtime("Can't filter pair");
	res.val.ad = clist_filter(fs->pool, v1.val.ad, v2, 1);
	break;

      default:
	bug("unknown Ca operation");
      }
    }
    else if (v1.type == T_ECLIST)
    {
      /* Extended community list */
      int arg_set = 0;

      /* v2.val is either EC or EC-set */
      if ((v2.type == T_SET) && eclist_set_type(v2.val.t))
	arg_set = 1;
      else if (v2.type == T_ECLIST)
	arg_set = 2;
      else if (v2.type != T_EC)
	runtime("Can't add/delete non-ec");

      res.type = T_ECLIST;
      switch (what->aux)
      {
      case 'a':
	if (arg_set == 1)
	  runtime("Can't add set");
	else if (!arg_set)
	  res.val.ad = ec_set_add(fs->pool, v1.val.ad, v2.val.ec);
	else
	  res.val.ad = ec_set_union(fs->pool, v1.val.ad, v2.val.ad);
	break;

      case 'd':
	if (!arg_set)
	  res.val.ad = ec_set_del(fs->pool, v1.val.ad, v2.val.ec);
	else
	  res.val.ad = eclist_filter(fs->pool, v1.val.ad, v2, 0);
	break;

      case 'f':
	if (!arg_set)
	  runtime("Can't filter ec");
	res.val.ad = eclist_filter(fs->pool, v1.val.ad, v2, 1);
	break;

      default:
	bug("unknown Ca operation");
      }
    }
    else if (v1.type == T_LCLIST)
    {
      /* Large community list */
      int arg_set = 0;

      /* v2.val is either LC or LC-set */
      if ((v2.type == T_SET) && lclist_set_type(v2.val.t))
	arg_set = 1;
      else if (v2.type == T_LCLIST)
	arg_set = 2;
      else if (v2.type != T_LC)
	runtime("Can't add/delete non-lc");

      res.type = T_LCLIST;
      switch (what->aux)
      {
      case 'a':
	if (arg_set == 1)
	  runtime("Can't add set");
	else if (!arg_set)
	  res.val.ad = lc_set_add(fs->pool, v1.val.ad, v2.val.lc);
	else
	  res.val.ad = lc_set_union(fs->pool, v1.val.ad, v2.val.ad);
	break;

      case 'd':
	if (!arg_set)
	  res.val.ad = lc_set_del(fs->pool, v1.val.ad, v2.val.lc);
	else
	  res.val.ad = lclist_filter(fs->pool, v1.val.ad, v2, 0);
	break;

      case 'f':
	if (!arg_set)
	  runtime("Can't filter lc");
	res.val.ad = lclist_filter(fs->pool, v1.val.ad, v2, 1);
	break;

      default:
	bug("unknown Ca operation");
      }
    }
    else
      runtime("Can't add/delete to non-[e|l]clist");

    break;

  case FI_ROA_CHECK:	/* ROA Check */
    if (what->arg1)
    {
      ARG(1, T_NET);
      ARG(2, T_INT);

      as = v2.val.i;
    }
    else
    {
      ACCESS_RTE;
      ACCESS_EATTRS;
      v1.val.net = (*fs->rte)->netA;

      /* We ignore temporary attributes, probably not a problem here */
      /* 0x02 is a value of BA_AS_PATH, we don't want to include BGP headers */
      eattr *e = ea_find(*fs->eattrs, EA_CODE(PROTOCOL_BGP, 0x02));

      if (!e || ((e->type & EAF_TYPE_MASK) != EAF_TYPE_AS_PATH))
	runtime("Missing AS_PATH attribute");

      as_path_get_last(e->u.ptr, &as);
    }

    struct rtable *table = ((struct f_inst_roa_check *) what)->rtc->table;
    if (!table)
      runtime("Missing ROA table");

    if (table->addr_type != NET_ROA4 && table->addr_type != NET_ROA6)
      runtime("Table type must be either ROA4 or ROA6");

    res.type = T_ENUM_ROA;

    if (table->addr_type != (v1.val.net->type == NET_IP4 ? NET_ROA4 : NET_ROA6))
      res.val.i = ROA_UNKNOWN; /* Prefix and table type mismatch */
    else
      res.val.i = net_roa_check(table, v1.val.net, as);

    break;

  case FI_FORMAT:	/* Format */
    ARG_ANY(1);

    res.type = T_STRING;
    res.val.s = val_format_str(fs, v1);
    break;

  case FI_ASSERT:	/* Birdtest Assert */
    ARG(1, T_BOOL);

    res.type = v1.type;
    res.val = v1.val;

    CALL(bt_assert_hook, res.val.i, what);
    break;

  default:
    bug( "Unknown instruction %d (%c)", what->fi_code, what->fi_code & 0xff);