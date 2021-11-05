CREATE TABLE public.property
(
    name          TEXT NOT NULL,
    instance_name TEXT NOT NULL,
    class_name    TEXT NOT NULL,
    process_name  TEXT NOT NULL,
    "type"        TEXT NOT NULL,
    value         TEXT NOT NULL,
    CONSTRAINT property_PK PRIMARY KEY (name, instance_name, class_name, process_name)
);
CREATE TABLE public.users
(
    first_name        TEXT                     NOT NULL,
    last_name         TEXT                     NOT NULL,
    e_mail            TEXT                     NOT NULL,
    user_name         TEXT                     NOT NULL,
    "password"        TEXT                     NOT NULL,
    created_timestamp timestamp with time zone NOT NULL,
    CONSTRAINT user_pk PRIMARY KEY (user_name)
);
CREATE TABLE public.groups
(
    "name"  text NOT NULL,
    virtual bool NOT NULL,
    CONSTRAINT group_pk PRIMARY KEY ("name")
);
CREATE TABLE public.user_groups_relation
(
    user_name  text NOT NULL,
    group_name text NOT NULL,
    CONSTRAINT user_groups_pk PRIMARY KEY (user_name, group_name),
    CONSTRAINT user_groups_fk FOREIGN KEY (group_name) REFERENCES public."groups" ("name"),
    CONSTRAINT user_groups_fk_1 FOREIGN KEY (user_name) REFERENCES public."users" (user_name)
);
CREATE TABLE public.group_groups_relation
(
    group_name      text NOT NULL,
    base_group_name text NOT NULL,
    CONSTRAINT group_groups_relation_pk PRIMARY KEY (group_name, base_group_name),
    CONSTRAINT group_groups_relation_fk FOREIGN KEY (group_name) REFERENCES public."groups" ("name"),
    CONSTRAINT group_groups_relation_fk_1 FOREIGN KEY (base_group_name) REFERENCES public."groups" ("name")
);
CREATE TABLE public.shared_memory_repositories
(
    shared_memory_segment    text    NOT NULL,
    shared_memory_repository text    NOT NULL,
    current_version          INTEGER NOT NULL,
    current_data_size        INTEGER NOT NULL,
    CONSTRAINT shared_memory_repositories_pk PRIMARY KEY (shared_memory_segment, shared_memory_repository)
);