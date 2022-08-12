See @ref array.

@defgroup array Arrays

The SSM runtime supports contiguous arrays of #ssm_value_t. These are managed
similarly to ADT objects (@ref adt), except (1) there is no tag object, and (2)
arrays may accommodate up to 65536 fields (instead of 256). This is
accomplished by interpreting the latter two fields of #ssm_mm as a single
16-bit size field; see #ssm_mm16.
