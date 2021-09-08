insert into public.user(first_name, last_name, e_mail, user_name, password, created_timestamp)
values ('Dominik', 'Krümpelmann', 'example@example.com', 'dominik',
        'REPLACE_WITH_NON_FUNCTIONAL_HASH_OR_SET_PASSWORD_MANUALLY',
        '2021-09-08 17:04:49.580723+02');
insert into public.group(name, virtual)
values ('User', false),
       ('Admin', false);
insert into public.user_groups_relation(user_name, group_name)
values ('dominik', 'User'),
       ('dominik', 'Admin');